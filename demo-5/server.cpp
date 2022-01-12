//16:20


//***************************************************************************
//
// Program example for labs in subject Operating Systems
//
// Petr Olivka, Dept. of Computer Science, petr.olivka@vsb.cz, 2017
//
// Example of socket server.
//
// This program is example of socket server and it allows to connect and serve
// the only one client.
// The mandatory argument of program is port number for listening.
//
//***************************************************************************

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>
#include <semaphore.h>

#define STR_CLOSE   "exit"
#define STR_QUIT    "quit"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

#define SEM_MUTEX_NAME      "/sem_mutexx"

sem_t* g_sem_mutex;

// debug flag
int g_debug = LOG_INFO;

int p_id;
int cmd_p_id;

void log_msg( int t_log_level, const char *t_form, ... )
{
    const char *out_fmt[] = {
            "ERR: (%d-%s) %s\n",
            "INF: %s\n",
            "DEB: %s\n" };

    if ( t_log_level && t_log_level > g_debug ) return;

    char l_buf[ 1024 ];
    va_list l_arg;
    va_start( l_arg, t_form );
    vsprintf( l_buf, t_form, l_arg );
    va_end( l_arg );

    switch ( t_log_level )
    {
    case LOG_INFO:
    case LOG_DEBUG:
        fprintf( stdout, out_fmt[ t_log_level ], l_buf );
        break;

    case LOG_ERROR:
        fprintf( stderr, out_fmt[ t_log_level ], errno, strerror( errno ), l_buf );
        break;
    }
}

//***************************************************************************
// help

void help( int t_narg, char **t_args )
{
    if ( t_narg <= 1 || !strcmp( t_args[ 1 ], "-h" ) )
    {
        printf(
            "\n"
            "  Socket server example.\n"
            "\n"
            "  Use: %s [-h -d] port_number\n"
            "\n"
            "    -d  debug mode \n"
            "    -h  this help\n"
            "\n", t_args[ 0 ] );

        exit( 0 );
    }

    if ( !strcmp( t_args[ 1 ], "-d" ) )
        g_debug = LOG_DEBUG;
}

//***************************************************************************

void* client_function(void* socket)
{
    int l_sock_client = *(int*)&socket;
    char l_buf[255];

    while(1)
    {
         // read data from socket
        int l_len = read( l_sock_client, l_buf, sizeof( l_buf ) );
        if ( !l_len )
        {
                log_msg( LOG_DEBUG, "Client closed socket!" );
                close( l_sock_client );
        }
        else if ( l_len < 0 )
                log_msg( LOG_DEBUG, "Unable to read data from client." );
        else
                log_msg( LOG_DEBUG, "Read %d bytes from client.", l_len );



        // close request?
        if ( !strncasecmp( l_buf, "exit", strlen( STR_CLOSE ) ) )
        {
                log_msg( LOG_INFO, "Client sent 'close' request to close connection." );
                close( l_sock_client );
                log_msg( LOG_INFO, "Connection closed. Waiting for new client." );
                break;
        }

        char* token;
        char** words = new char*[10];

        token = strtok(l_buf, " \n");

        int counter = 0;
        while(token != NULL) 
        {
            words[counter++] = token;
            token = strtok(NULL, " \n");
        }

        int pipefd[2];
        pipe(pipefd);

        cmd_p_id = fork();

        if(cmd_p_id == 0)
        {
            close(pipefd[0]);    // close reading end in the child

            dup2(pipefd[1], 1);  // send stdout to the pipe
            dup2(pipefd[1], 2);  // send stderr to the pipe

            close(pipefd[1]);    // this descriptor is no longer needed
            
            sem_wait(g_sem_mutex);
            execvp(words[0], words);
            
        }
        else
        {
            int l_status;
            waitpid( cmd_p_id, &l_status, 0 );
            sem_post(g_sem_mutex);
            char output_buffer[255];

            close(pipefd[1]);  // close the write end of the pipe in the parent

            while (read(pipefd[0], output_buffer, sizeof(output_buffer)) != 0)
            {
            }

            char str[2] = "\0"; /* gives {\0, \0} */
            str[0] = '\n';
    
            strcat(output_buffer, str);

            // write data to client
            l_len = write( l_sock_client, output_buffer, strlen(output_buffer) );
            if ( l_len < 0 )
                    log_msg( LOG_ERROR, "Unable to write data to stdout." );

            // close(l_sock_client);
            // exit( 0 );
            memset(output_buffer, 0, strlen(output_buffer));
            bzero(l_buf, sizeof(l_buf));
        }
    }
}


int main( int t_narg, char **t_args )
{
    if ( t_narg <= 1 ) help( t_narg, t_args );

    int l_port = 0;

    // parsing arguments
    for ( int i = 1; i < t_narg; i++ )
    {
        if ( !strcmp( t_args[ i ], "-d" ) )
            g_debug = LOG_DEBUG;

        if ( !strcmp( t_args[ i ], "-h" ) )
            help( t_narg, t_args );

        if ( *t_args[ i ] != '-' && !l_port )
        {
            l_port = atoi( t_args[ i ] );
            break;
        }
    }

    if ( l_port <= 0 )
    {
        log_msg( LOG_INFO, "Bad or missing port number %d!", l_port );
        help( t_narg, t_args );
    }

    log_msg( LOG_INFO, "Server will listen on port: %d.", l_port );

    // socket creation
    int l_sock_listen = socket( AF_INET, SOCK_STREAM, 0 );
    if ( l_sock_listen == -1 )
    {
        log_msg( LOG_ERROR, "Unable to create socket.");
        exit( 1 );
    }

    in_addr l_addr_any = { INADDR_ANY };
    sockaddr_in l_srv_addr;
    l_srv_addr.sin_family = AF_INET;
    l_srv_addr.sin_port = htons( l_port );
    l_srv_addr.sin_addr = l_addr_any;

    // Enable the port number reusing
    int l_opt = 1;
    if ( setsockopt( l_sock_listen, SOL_SOCKET, SO_REUSEADDR, &l_opt, sizeof( l_opt ) ) < 0 )
      log_msg( LOG_ERROR, "Unable to set socket option!" );

    // assign port number to socket
    if ( bind( l_sock_listen, (const sockaddr * ) &l_srv_addr, sizeof( l_srv_addr ) ) < 0 )
    {
        log_msg( LOG_ERROR, "Bind failed!" );
        close( l_sock_listen );
        exit( 1 );
    }

    // listenig on set port
    if ( listen( l_sock_listen, 1 ) < 0 )
    {
        log_msg( LOG_ERROR, "Unable to listen on given port!" );
        close( l_sock_listen );
        exit( 1 );
    }

    log_msg( LOG_INFO, "Enter 'quit' to quit server." );

    int l_sock_client;

    g_sem_mutex = sem_open( SEM_MUTEX_NAME, O_RDWR | O_CREAT, 0660, 1 );
    sem_init(g_sem_mutex, 1 , 1);

    // go!
    while ( 1 )
    {
        sockaddr_in l_rsa;
        int l_rsa_size = sizeof( l_rsa );
        // new connection
        l_sock_client = accept( l_sock_listen, ( sockaddr * ) &l_rsa, ( socklen_t * ) &l_rsa_size );
        if ( l_sock_client == -1 )
        {
            log_msg( LOG_ERROR, "Unable to accept new client." );
            close( l_sock_listen );
            exit( 1 );
        }


        p_id = fork();


        if(p_id == 0)
            client_function((void*)l_sock_client);
        
    } // while ( 1 )

    return 0;
}