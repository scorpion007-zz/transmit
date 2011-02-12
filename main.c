// Copyright (c) 2011 Alex Budovski.

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdarg.h>

// NOTE: TransmitFile fails to send > 2GB in one go. So we need to send in ~2GB
// chunks.
#define TWO_GB (0x80000000-512)

#ifdef _DEBUG
#define DBGPRINT debug
#else
#define DBGPRINT
#endif

void error(const char *fmt, ...) {
  va_list args;
  fprintf(stderr, "error: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void debug(const char *fmt, ...) {
  va_list args;
  fprintf(stderr, "debug: ");
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

void print_usage(char *image) {
  printf("transmit a file through a socket.\n\n"
         "usage: %s <file> <host> <port>\n\n"
         "where\n\n"
         "<file>\tthe full path to file to be transmitted or '-' for stdin\n"
         "<host>\thostname or ipv4 address of target\n"
         "<port>\tservice port target is listening on\n", image);
}

int main(int argc, char **argv) {
  WSADATA wsaData;
  OVERLAPPED overlapped = {0};
  LARGE_INTEGER file_size, offset = {0};
  int err;

  // command-line args
  const char *filename;
  const char *hostname;
  const char *port;

  SOCKET s;
  HANDLE hFile;           // file to read
  SOCKADDR_IN host;       // destination address

  // used for dns lookup.
  struct addrinfo aiHints = {0};
  struct addrinfo *aiList = NULL;

  if (argc < 4) {
    print_usage(argv[0]);
    return 1;
  }

  // Parse command line.
  filename = argv[1];
  hostname = argv[2];
  port = argv[3];

  if (!strcmp(filename, "-")) {
    // Use stdin.
    hFile = GetStdHandle(STD_INPUT_HANDLE);
  } else {
    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
      error("failed to open file: %s\n", filename);
      return 1;
    }
  }

  err = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if ( err != 0 ) {
    /* Tell the user that we could not find a usable */
    /* WinSock DLL.                                  */
    error("failed to init WSA\n");
    return 1;
  }

  /* Confirm that the WinSock DLL supports 2.2.*/
  /* Note that if the DLL supports versions greater    */
  /* than 2.2 in addition to 2.2, it will still return */
  /* 2.2 in wVersion since that is the version we      */
  /* requested.                                        */

  if (LOBYTE( wsaData.wVersion ) != 2 ||
      HIBYTE( wsaData.wVersion ) != 2 ) {
    /* Tell the user that we could not find a usable */
    /* WinSock DLL.                                  */
    error("failed to obtain requested version\n");
    WSACleanup( );
    return 1;
  }

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  // request only TCP/IPv4-streaming sockets.
  aiHints.ai_family = AF_INET;
  aiHints.ai_socktype = SOCK_STREAM;
  aiHints.ai_protocol = IPPROTO_TCP;

  err = getaddrinfo(hostname, port, &aiHints, &aiList);
  if (err) {
    error("getaddrinfo failed: %s\n", gai_strerror(err));
  }

  // use the first SOCKADDR returned in the list.
  host = *(SOCKADDR_IN *)aiList->ai_addr;

  // free the dynamically allocated addrinfo structure.
  freeaddrinfo(aiList);

  // Connect to server.
  if (connect(s, (SOCKADDR*)&host, sizeof(host)) == SOCKET_ERROR) {
    error("failed to connect.\n");
    closesocket(s);
    WSACleanup();
    return 1;
  }

  if (!GetFileSizeEx(hFile, &file_size)) {
    error("GetFileSize failed: %d\n", GetLastError());
    return 1;
  }

  overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

  // send the file over.
  while (file_size.QuadPart > 0) {
    if (!TransmitFile(s, hFile, (DWORD)min(TWO_GB, file_size.QuadPart),
                      0, &overlapped, NULL, TF_USE_KERNEL_APC)) {
      err = WSAGetLastError();
      if (err == WSA_IO_PENDING) {
        // everything's OK, just hasn't completed yet.
        DWORD ret = WaitForSingleObject(overlapped.hEvent, INFINITE);
        if (ret == WAIT_FAILED) {
          error("wait failed: %d\n", GetLastError());
          return 1;
        }
      } else {
        error("failed to send file: %d\n", err);
        return 1;
      }
    }
    file_size.QuadPart -= TWO_GB;
    DBGPRINT("sent %d bytes, remain: %I64d\n", TWO_GB, file_size.QuadPart);
    offset.QuadPart += TWO_GB;
    overlapped.Offset = offset.LowPart;
    overlapped.OffsetHigh = offset.HighPart;
  }

  CloseHandle(overlapped.hEvent);
  closesocket(s);
  WSACleanup();
  return 0;
}
