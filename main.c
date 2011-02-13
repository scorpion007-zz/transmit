// Copyright (c) 2011 Alex Budovski.

#include <winsock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdarg.h>

// NOTE: TransmitFile fails to send > 2GB in one go.
#define CHUNK_SIZE (2*1024*1024)

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
  BOOL usingStdin = FALSE;
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
    usingStdin = TRUE;
  } else {
    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ,
                       NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
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

  if (usingStdin) {
    // Can't use TransmitFile on pipes. (stdin)
    char buf[64*1024];
    DWORD nread;
    while ((err = ReadFile(hFile, buf, sizeof buf, &nread, NULL)) != 0) {
      if (nread == 0) {  // at EOF
        break;
      }
      if (send(s, buf, nread, 0) == SOCKET_ERROR) {
        error("send failed: %d\n", WSAGetLastError());
        return 1;
      }
    }

    if (!err) {
      err = GetLastError();
      if (err != ERROR_BROKEN_PIPE) {  // broken pipe expected.
        error("ReadFile failed: %d\n", err);
        return 1;
      }
    }
  } else {
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(hFile, &file_size)) {
      error("GetFileSize failed: %d\n", GetLastError());
      return 1;
    }

    // send the file over.
    while (file_size.QuadPart > 0) {
      if (!TransmitFile(s, hFile, (DWORD)min(CHUNK_SIZE, file_size.QuadPart),
            0, NULL, NULL, TF_USE_KERNEL_APC)) {
        error("failed to send file: %d\n", WSAGetLastError());
        return 1;
      }
      file_size.QuadPart -= CHUNK_SIZE;
      DBGPRINT("sent %d bytes, remain: %I64d\n", CHUNK_SIZE, file_size.QuadPart);
    }
    CloseHandle(hFile);
  }

  closesocket(s);
  WSACleanup();
  return 0;
}
