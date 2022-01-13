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

#define STR_CLOSE   "close"
#define STR_QUIT    "quit"

//***************************************************************************
// log messages

#define LOG_ERROR               0       // errors
#define LOG_INFO                1       // information and notifications
#define LOG_DEBUG               2       // debug messages

// debug flag
int g_debug = LOG_INFO;

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

void* process_function(int l_sock_client)
{

    while(1)
    {
        char l_buf[255];

        // read data from socket
        int l_len = read( l_sock_client, l_buf, sizeof( l_buf ) );
        if ( !l_len )
        {
                log_msg( LOG_DEBUG, "Client closed socket!" );
                close( l_sock_client );
                break;
        }
        else if ( l_len < 0 )
                log_msg( LOG_DEBUG, "Unable to read data from client." );
        else
                log_msg( LOG_DEBUG, "Read %d bytes from client.", l_len );


        char get_string[32];
        char command[32];
        char http_string[32];
        char rest[256];

        sscanf(l_buf, "%[A-Za-z] /%[A-Za-z] %[A-Za-z]", get_string, command, http_string);


        if(strncmp(get_string, "GET", 3) != 0 || strncmp(http_string, "HTTP", 4) != 0)
        {
            close(l_sock_client);
            break;
        }
        memset(l_buf, 0, sizeof(l_buf));

        // int l_mypipe[ 2 ];
        // if ( pipe( l_mypipe ) < 0 )
        // {
        //     log_msg( LOG_ERROR, "Unable to create pipe!" );
        //     exit( 1 );
        // }

        int p_id = fork();

        if(p_id == 0)
        {
            // close( l_mypipe[ 0 ] );

            // dup2( l_mypipe[ 1 ], 1);
            // dup2( l_mypipe[ 1 ], 2);

            // dup2(STDOUT_FILENO, l_sock_client);
            dup2 (l_sock_client, STDOUT_FILENO);
            // close( l_mypipe[ 1 ] );
            // dup2(STDOUT_FILENO, l_sock_client);
            close(l_sock_client);
            execlp(command, command, nullptr);

            exit(1);
        }
        else
        {
            int l_status;
            waitpid(p_id, &l_status, 0);

            // printf("%s %s %s\n", get_string, command, http_string);
    
            // int err = read( l_mypipe[ 0 ], l_buf, sizeof( l_buf ) );
            // if ( err < 0 )
            // {
            //     log_msg( LOG_ERROR, "Function read failed!" );
            // }
            // if ( err <= 0 )
            // {
            //     break;
            // }

            // printf("%s", l_buf);

            // write data to client
            // l_len = write( l_sock_client, l_buf, strlen(l_buf) );
            // if ( l_len < 0 )
            //         log_msg( LOG_ERROR, "Unable to write data to stdout." );

            // close request?
            if ( !strncasecmp( l_buf, "close", strlen( STR_CLOSE ) ) )
            {
                    log_msg( LOG_INFO, "Client sent 'close' request to close connection." );
                    close( l_sock_client );
                    log_msg( LOG_INFO, "Connection closed. Waiting for new client." );
                    break;
            }
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

        if(fork() == 0)
        {
            // close(l_sock_listen);
            process_function(l_sock_client);
        }
    } // while ( 1 )
    close(l_sock_listen);


    return 0;
}