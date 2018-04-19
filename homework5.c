#include <fnmatch.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/mman.h>
#include <dirent.h>
#include <pthread.h>

#define BACKLOG (10)

void serve_request(int);
char* substring(char*, int, int);
//char* errorpage(int);


// THIS GOT UGLY REALLY QUICKLY SORRY
char * request_str_200_html = "HTTP/1.0 200 OK\r\nContent-type: text/html; charset=UTF-8\r\n\r\n";
char * request_str_200_jpg = "HTTP/1.0 200 OK\r\nContent-type: image/jpeg; charset=UTF-8\r\n\r\n";
char * request_str_200_png = "HTTP/1.0 200 OK\r\nContent-type: image/png; charset=UTF-8\r\n\r\n";
char * request_str_200_gif = "HTTP/1.0 200 OK\r\nContent-type: image/gif; charset=UTF-8\r\n\r\n";
char * request_str_200_pdf = "HTTP/1.0 200 OK\r\nContent-type: application/pdf; charset=UTF-8\r\n\r\n";
char * request_str_400 = "HTTP/1.0 400 Bad Request\r\nContent-type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n";
char * request_str_404 = "HTTP/1.0 404 Not Found\r\nContent-type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n";

// snprintf(output_buffer,4096,index_hdr,filename,filename);
char * index_body = "<li><a href=\"%s\">%s</a>";
char * index_ftr = "</ul><hr></body></html>";

// ==================== ERROR 400 on my ERROR 400 ==============================
/*char *errorpage(int flag)
{
  char * p404 = "<!DOCTYPE html><html><head><title>Lab 4 test pages: ERROR 404</title></head>";
  strcat(p404, "<body style='font-family:Garamond'><center><h1 style='color:red; font-size:70px; margin-bottom: -5px;'>The limit does not exist.</h1>\r\n");
  strcat(p404, "<div style='padding: -20px; margin-bottom: -10px; font-size:20px'><b>And neither does this page sadly.</b>\r\n");
  strcat(p404, "<div style='font-size:16px;'>This is a 404 error no-no. Try again!</div></div>");
  strcat(p404, "</center></body></html>");

  char * p400 = "<!DOCTYPE html><html><head><title>Lab 4 test pages: ERROR 400</title></head>";
  strcat(p400, "<body style='font-family:Garamond'><center><h1 style='color:red; font-size:70px; margin-bottom: -5px;'>Something's wrong, and it's not us.</h1>\r\n");
  strcat(p400, "<div style='padding: -20px; margin-bottom: -10px; font-size:20px'><b>We can't fix everyone else's problems you know.</b>\r\n");
  strcat(p400, "<div style='font-size:16px;'>This is a 400 error no-no. Try again!</div></div>");
  strcat(p400, "</center></body></html>");
  

  if (flag == 404)
    return p404;
  else
    return p400;
} */

// =================== for myself because I suck ==============
char *substring(char *string, int position, int length) 
{
   char *pointer;
   int c;
 
   pointer = malloc(length+1);
 
   if (pointer == NULL)
   {
      printf("Unable to allocate memory.\n");
      exit(1);
   }
 
   for (c = 0 ; c < length ; c++)
   {
      *(pointer+c) = *(string+position-1);      
      string++;   
   }
 
   *(pointer+c) = '\0';
 
   return pointer;
}

// ============ DIRECTORY HANDLING ==========

#define PATH_ARG 1
#define DIRECTORY_LISTING_MAX_CHAR 1013 // can get overflow if have a large directory and not enough characters

// checks whether index
int is_index(char* directory_path)
{ 
  // open directory path up 
  DIR* path = opendir(directory_path);

  // check to see if opening up directory was successful
  if(path != NULL)
  {
      // stores underlying info of files and sub_directories of directory_path
      struct dirent* underlying_file = NULL;

      while((underlying_file = readdir(path)) != NULL)
      {
          char* ufile = underlying_file->d_name;
          if (strcmp(ufile, "index.html") == 0)
          {
            return 0;
          }
          
      }
      closedir(path);
  }
  return 1;

}

// return directory paging html page
char* get_directory_contents(char* directory_path)
{
  char* directory_listing = NULL;
  
  // open directory path up 
  DIR* path = opendir(directory_path);

  char * index_hdr = "<!DOCTYPE html><html><title>Directory Listing</title>";

  // check to see if opening up directory was successful
  if(path != NULL)
  {
      directory_listing = (char*) malloc(sizeof(char)*DIRECTORY_LISTING_MAX_CHAR);
      directory_listing[0] = '\0';

      // stores underlying info of files and sub_directories of directory_path
      struct dirent* underlying_file = NULL;

      // iterate through all of the  underlying files of directory_path
      strcat(directory_listing, index_hdr);
      strcat(directory_listing, "<body style='font-family:Garamond;'>");
      strcat(directory_listing, "<h1 style='color:red; font-size:32px; margin-bottom: -5px;'>Directory Listing</h1>");
      strcat(directory_listing, "<h3 style='color:#6d1c14; font-size:24px; margin-bottom: -5px;margin-top: 5px'>");
      strcat(directory_listing, directory_path);
      strcat(directory_listing, "</h3><ul style='font-size:20px'>");

      while((underlying_file = readdir(path)) != NULL)
      {
          char* ufile = underlying_file->d_name;

          // avoids directory not having / in the end if not a file
          //int needSlash = 1;
          //char* subufile = substring(directory_path, strlen(directory_path), 1);
          //printf("[[SUBFILE: %s]]\n", subufile);
          /*if (strcmp(subufile, "/") != 0)
          {
            // if we are trying to access file, append before
            if (strstr(ufile, ".jpg") != NULL || strstr(ufile, ".jpeg") != NULL ||
              strstr(ufile, ".png") != NULL || strstr(ufile, ".gif") != NULL ||
              strstr(ufile, ".pdf") != NULL || strstr(ufile, ".html") != NULL)
            {
              if (strcmp("..", ufile) != 0 && strcmp(".", ufile) != 0)
              {
                needSlash = 0;
              }
            }
          }*/

          //printf("[[NEW SUBFILE: %s]]\n", ufile);
          strcat(directory_listing, "<li><a href='");
          /*if (needSlash == 0)
            strcat(directory_listing, "./");*/
          strcat(directory_listing, ufile);
          strcat(directory_listing, "'>");
          strcat(directory_listing, ufile);
          strcat(directory_listing, "</a></li>");
      }
      strcat(directory_listing, "</ul></body></html>");
      
      closedir(path);
  }

  return directory_listing;
}

int directory_handler(char* directory)
{
  directory = substring(directory, 2, strlen(directory) - 1);
  //printf("[[Checking directory %s]]\n\n", directory);
  //check if file exists
  struct stat file_stat;
  if (stat(directory, &file_stat) != 0) {
    // if it deals with a file, 404 error; otherwise, 400 error
    if (strstr(directory, ".jpg") != NULL && strstr(directory, ".jpeg") != NULL &&
      strstr(directory, ".png") != NULL && strstr(directory, ".gif") != NULL &&
      strstr(directory, ".pdf") != NULL  && strstr(directory, ".html") != NULL )
    {
      if (strcmp(directory, "") == 0)
        return 206; // main directory
      else
        return 404;
    }
    else
    {
      return 400;
    }
  }

  //check if file is a directory
  if (S_ISDIR(file_stat.st_mode)) {
    // is a directory, read in contents and check for read in errors
    char* directory_contents_str = get_directory_contents(directory); 
    if(directory_contents_str != NULL)
    {
      // check if tindex.html is in directory, otherwise return directory page
      if (is_index(directory) == 0)
        return 201;
      else
        return 206;
    }
    else
    {
      // empty directory
      //fprintf(stderr, "Error reading contents of %s\n", directory[PATH_ARG]);
      return 400;
    }
  } else {
      if (strstr(directory, ".jpg") != NULL || strstr(directory, ".jpeg") != NULL )
      {
        return 202;
      }
      else if (strstr(directory, ".png") != NULL )
      {
        return 203;
      }
      else if (strstr(directory, ".gif") != NULL )
      {
        return 204;
      }
      else if (strstr(directory, ".pdf") != NULL )
      {
        return 205;
      }
      else if (strstr(directory, ".html") != NULL )
      {
        return 200;
      }
      else if (strcmp(directory, "") == 0)
      {
        return 206;
      }
      else
      {
        return 400;
      }
  }


  return 0;
}

//================================================================

// GRABS FILENAME
char* parseRequest(char* request) {
  char *buffer = malloc(sizeof(char)*257);
  memset(buffer, 0, 257);
  
  // request matches, return 0;
  if(fnmatch("GET * HTTP/1.*",  request, 0)) return 0; 

  // gets filename
  sscanf(request, "GET %s HTTP/1.", buffer);
  return buffer; 
}


void serve_request(int client_fd){
  int read_fd;
  int bytes_read;

  // buffers
  int file_offset = 0;
  char client_buf[4096];
  char send_buf[4096];
  char filename[4096];
  char * requested_file;
  char * header;

  // zero out buffers
  memset(client_buf,0,4096);
  memset(filename,0,4096);

  // read request from file or get header into client_buf
  while(1){
    file_offset += recv(client_fd,&client_buf[file_offset],4096,0);
    if(strstr(client_buf,"\r\n\r\n"))
      break;
  }

  // grabs directory
  requested_file = parseRequest(client_buf);
  //printf("[[client_buf:]] %s \n", client_buf);

  // int code = rightHeader(requested_file);
  int code = directory_handler(requested_file);
  printf("[[initial requested file]]: %s\n", requested_file);
  // grabs right header and file from code
  switch (code)
  {
    case 200:
      header = request_str_200_html;
      break;
    case 201:
      strcat(requested_file, "/index.html");
      header = request_str_200_html;
      break;
    case 202:
      header = request_str_200_jpg;
      break;
    case 203:
      header = request_str_200_png;
      break;
    case 204:
      header = request_str_200_gif;
      break;
    case 205:
      header = request_str_200_pdf;
      break;
    case 206:
      header = request_str_200_html;
      requested_file = get_directory_contents(substring(requested_file, 2, strlen(requested_file) - 1));
      break;
    case 400:
      //requested_file = errorpage(400);
      requested_file = "/400.html";
      header = request_str_400;
      //header = request_str_200_html;
      break;
    case 404:
      //requested_file = errorpage(404);
      requested_file = "/404.html";
      header = request_str_404;
      //header = request_str_200_html;
      break;
  }
  // console.log()
  printf("[[requested_file:]] %s \n", requested_file);
  printf("[[CODE SENT]] %i \n", code);

  // send header
  send(client_fd,header,strlen(header),0);

  //if(code == 206 || code == 400 || code == 404)
  if (code == 206)
  {
    send(client_fd, requested_file, strlen(requested_file),0);
  }
  else
  {
    // take requested_file, add a . to beginning, open that file
    filename[0] = '.';
    strncpy(&filename[1],requested_file,4095);
    printf("[[opened requested_file:]] %s \n", filename);
    read_fd = open(filename,0,0);

    // deals with the file we are sending
    while(1){
      bytes_read = read(read_fd,send_buf,4096);
      if(bytes_read == 0)
        break;

      send(client_fd,send_buf,bytes_read,0);
    }
    close(read_fd);
  }

  close(client_fd);
  return;
}

int main(int argc, char** argv) {
    /* For checking return values. */
    int retval;
    chdir(argv[2]);

    /* Read the port number from the first command line argument. */
    int port = atoi(argv[1]);

    /* Create a socket to which clients will connect. */
    int server_sock = socket(AF_INET6, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    // rebind immediately to port
    int reuse_true = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_true,
                        sizeof(reuse_true));
    if (retval < 0) {
        perror("Setting socket option failed");
        exit(1);
    }

    // create address structure
    struct sockaddr_in6 addr;   // internet socket address data structure
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(port); // byte order is significant
    addr.sin6_addr = in6addr_any; // listen to all interfaces

    // bind socket
    retval = bind(server_sock, (struct sockaddr*)&addr, sizeof(addr));
    if(retval < 0) {
        perror("Error binding to port");
        exit(1);
    }

    // listen
    retval = listen(server_sock, BACKLOG);
    if(retval < 0) {
        perror("Error listening for connections");
        exit(1);
    }

    pid_t pid;
    //p_thread_t thread_id;

    while(1) {
        /* Declare a socket for the client connection. */
        int sock;
        //char buffer[256];

        // sighandlers
        //(void) signal(SIGINT, cleanup);
        //(void) signal(SIGSTP, cleanup);

        // remote socket address
        struct sockaddr_in remote_addr;
        unsigned int socklen = sizeof(remote_addr); 

        // Accept the first waiting connection from the server socket and
        // populate the address information.
        // sock = connected socket
        sock = accept(server_sock, (struct sockaddr*) &remote_addr, &socklen);
        if(sock < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        // connection handler is everything after this line
        /*int ptc = pthread_create(&thread_id, NULL, connection_handler, (void*) &sock);
        if (ptc < 0)
        {
          perror("Error in threading");
          exit(1);
        }*/

        /* ALWAYS check the return value of send().  Also, don't hardcode
         * values.  This is just an example.  Do as I say, not as I do, etc. */
        
        // fork connection
        pid = fork();

        if(pid == -1)
        {
            perror("Error forking");
            exit (1);
        }
        if (pid== 0)
        {
          close(server_sock);
          serve_request(sock);
          close(sock);
          exit(0);
        }

        // cleanup
        close(sock);
    }

    close(server_sock);
}