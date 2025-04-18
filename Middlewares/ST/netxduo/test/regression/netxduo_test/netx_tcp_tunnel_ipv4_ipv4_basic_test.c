/* This NetX IPsec basic test using AES.  */

#include   "tx_api.h"
#include   "nx_api.h"
#include   "nx_tcp.h"
extern void    test_control_return(UINT status);
#if defined(NX_TUNNEL_ENABLE) && !defined(NX_DISABLE_IPV4)
#include   "nx_tunnel.h"
#define     DEMO_STACK_SIZE         4096

#define MSG "abcdefghijklmnopqrstuvwxyz"

/* Define the ThreadX and NetX object control blocks...  */

static TX_THREAD               thread_0;
static TX_THREAD               thread_1;

static NX_PACKET_POOL          pool_0;
static NX_IP                   ip_0;
static NX_IP                   ip_1;
static NX_TCP_SOCKET           client_socket;
static NX_TCP_SOCKET           server_socket;

/* Define the counters used in the demo application...  */

static ULONG                   thread_0_counter =  0;
static ULONG                   thread_1_counter =  0;
static ULONG                   error_counter =     0;
static ULONG                   connections =       0;
static ULONG                   disconnections =    0;
static ULONG                   client_receives =   0;
static ULONG                   server_receives =   0;
static UINT                    client_port;
static CHAR                    rcv_buffer[200];


/* Define thread prototypes.  */

static void    thread_0_entry(ULONG thread_input);
static void    thread_1_entry(ULONG thread_input);
static void    thread_1_connect_received(NX_TCP_SOCKET *server_socket, UINT port);
static void    thread_1_disconnect_received(NX_TCP_SOCKET *server_socket);

static void    thread_0_receive_notify(NX_TCP_SOCKET *client_socket);
static void    thread_1_receive_notify(NX_TCP_SOCKET *server_socket);
extern void    test_control_return(UINT status);
extern void    _nx_ram_network_driver_1500(struct NX_IP_DRIVER_STRUCT *driver_req);

static NX_ADDRESS_SELECTOR   address_selector_0;
static NX_ADDRESS_SELECTOR   address_selector_1;
static NX_TUNNEL             tunnel_0;
static NX_TUNNEL             tunnel_1;

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void    netx_tcp_tunnel_ipv4_ipv4_basic_test_application_define(void *first_unused_memory)
#endif
{

CHAR    *pointer;
UINT    status; 
    
    /* Setup the working pointer.  */
    pointer =  (CHAR *) first_unused_memory;

    thread_0_counter =  0;
    thread_1_counter =  0;
    error_counter =     0;
    connections =       0;
    disconnections =    0;
    client_receives =   0;
    server_receives =   0;
    client_port =       0;

    /* Create the main thread.  */
    tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;

    /* Create the main thread.  */
    tx_thread_create(&thread_1, "thread 1", thread_1_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            4, 4, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;


    /* Initialize the NetX system.  */
    nx_system_initialize();

    /* Create a packet pool.  */
    status =  nx_packet_pool_create(&pool_0, "NetX Main Packet Pool", 512, pointer, 8192);
    pointer = pointer + 8192;

    if (status)
        error_counter++;


    /* Create an IP instance.  */
    status = nx_ip_create(&ip_0, "NetX IP Instance 0", IP_ADDRESS(1,2,3,4), 0xFFFFFF00UL, &pool_0, _nx_ram_network_driver_1500,
        pointer, 2048, 1);
    pointer = pointer + 2048;

    /* Create another IP instance.  */
    status += nx_ip_create(&ip_1, "NetX IP Instance 1", IP_ADDRESS(1,2,3,5), 0xFFFFFF00UL, &pool_0, _nx_ram_network_driver_1500,
        pointer, 2048, 1);
    pointer = pointer + 2048;


    status += nx_ip_interface_attach(&ip_0,"Second Interface",IP_ADDRESS(2,2,3,4),0xFFFFFF00UL,  _nx_ram_network_driver_1500);
    status += nx_ip_interface_attach(&ip_1,"Second Interface",IP_ADDRESS(2,2,3,5),0xFFFFFF00UL,  _nx_ram_network_driver_1500);

    if (status)
        error_counter++;

    /* Enable ARP and supply ARP cache memory for IP Instance 0.  */
    status =  nx_arp_enable(&ip_0, (void *) pointer, 1024);
    pointer = pointer + 1024;

    /* Enable ARP and supply ARP cache memory for IP Instance 1.  */
    status +=  nx_arp_enable(&ip_1, (void *) pointer, 1024);
    pointer = pointer + 1024;

    /* Check ARP enable status.  */
    if (status)
        error_counter++;

    /* Enable TCP processing for both IP instances.  */
    status =  nx_tcp_enable(&ip_0);
    status += nx_tcp_enable(&ip_1);

    /* Check TCP enable status.  */
    if (status)
        error_counter++;


    /* Enable ICMP for IP Instance 0 and 1.  */
    status = nxd_icmp_enable(&ip_0);
    status = nxd_icmp_enable(&ip_1);

    status = nx_tunnel_enable(&ip_0);
    status += nx_tunnel_enable(&ip_1);

    /* Check Tunnel enable status.  */
    if (status)
        error_counter++;
}



/* Define the test threads.  */

static void    thread_0_entry(ULONG thread_input)
{

UINT        status;
NX_PACKET   *my_packet;
CHAR        *msg = MSG;

    /* Print out some test information banners.  */
    printf("NetX Test:   TUNNEL TCP IPV4_4 Basic Processing Test.......");

    /* Check for earlier error.  */
    if (error_counter)
    {

        printf("ERROR!\n");
        test_control_return(1);
    }

    /* Create TUNNEL.  */
    address_selector_0.nx_selector_src_address_start.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_0.nx_selector_src_address_start.nxd_ip_address.v4 = 0x01000000;

    address_selector_0.nx_selector_src_address_end.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_0.nx_selector_src_address_end.nxd_ip_address.v4 = 0x02000000;

    address_selector_0.nx_selector_dst_address_start.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_0.nx_selector_dst_address_start.nxd_ip_address.v4 = 0x01000000;

    address_selector_0.nx_selector_dst_address_end.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_0.nx_selector_dst_address_end.nxd_ip_address.v4 = 0x02000000;

    /* add tunnel address.  */
    address_selector_0.nx_selector_src_tunnel_address.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_0.nx_selector_src_tunnel_address.nxd_ip_address.v4 = 0x02020304;
    address_selector_0.nx_selector_dst_tunnel_address.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_0.nx_selector_dst_tunnel_address.nxd_ip_address.v4 = 0x02020305;

    /* Set up TUNNEL */
    status = nx_tunnel_create(&ip_0, &tunnel_0,NX_IP_VERSION_V4,address_selector_0);

    if (status)
        error_counter++;

    /* Get a free port for the client's use.  */
    status =  nx_tcp_free_port_find(&ip_0, 1, &client_port);

    /* Check for error.  */
    if (status)
    {

        printf("ERROR!\n");
        test_control_return(1);
    }


    /* Increment thread 0's counter.  */
    thread_0_counter++;

    /* Create a socket.  */
    status =  nx_tcp_socket_create(&ip_0, &client_socket, "Client Socket", 
        NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 200,
        NX_NULL, NX_NULL);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Bind the socket.  */
    status =  nx_tcp_client_socket_bind(&client_socket, client_port, NX_WAIT_FOREVER);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Attempt to connect the socket.  */
    tx_thread_relinquish();

    status =  nx_tcp_client_socket_connect(&client_socket, IP_ADDRESS(1, 2, 3, 5), 12, 5 * NX_IP_PERIODIC_RATE);

    /* Check for error.  */
    if (status)
        error_counter++;


    /* Wait for established state.  */
    status =  nx_tcp_socket_state_wait(&client_socket, NX_TCP_ESTABLISHED, 5 * NX_IP_PERIODIC_RATE);

    /* Allocate a packet.  */
    status =  nx_packet_allocate(&pool_0, &my_packet, NX_TCP_PACKET, NX_WAIT_FOREVER);

    /* Check status.  */
    if (status != NX_SUCCESS)
        error_counter++;

    /* Write ABCs into the packet payload!  */
    memcpy(my_packet -> nx_packet_prepend_ptr, &msg[0], 26);

    /* Adjust the write pointer.  */
    my_packet -> nx_packet_length =  26;
    my_packet -> nx_packet_append_ptr =  my_packet -> nx_packet_prepend_ptr + 26;

    /* Suspend thread1 */
    tx_thread_suspend(&thread_1);

    /* Send the packet out!  */
    status =  nx_tcp_socket_send(&client_socket, my_packet, 2 * NX_IP_PERIODIC_RATE);

    /* Determine if the status is valid.  */
    if (status)
    {
        error_counter++;
        nx_packet_release(my_packet);
    }

    tx_thread_resume(&thread_1);    

    tx_thread_relinquish();   

    /* Disconnect this socket.  */
    status =  nx_tcp_socket_disconnect(&client_socket, 5 * NX_IP_PERIODIC_RATE);

    /* Determine if the status is valid.  */
    if (status)
        error_counter++;

    /* Unbind the socket.  */
    status =  nx_tcp_client_socket_unbind(&client_socket);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Delete the socket.  */
    status =  nx_tcp_socket_delete(&client_socket);

    /* Check for error.  */
    if (status)
        error_counter++;

}
    

static void    thread_1_entry(ULONG thread_input)
{

UINT            status;
NX_PACKET       *packet_ptr;
ULONG           actual_status;
ULONG           recv_length = 0;
    
    /* Create TUNNEL.  */
    address_selector_1.nx_selector_src_address_start.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_1.nx_selector_src_address_start.nxd_ip_address.v4 = 0x01000000;

    address_selector_1.nx_selector_src_address_end.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_1.nx_selector_src_address_end.nxd_ip_address.v4 = 0x02000000;

    address_selector_1.nx_selector_dst_address_start.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_1.nx_selector_dst_address_start.nxd_ip_address.v4 = 0x01000000;

    address_selector_1.nx_selector_dst_address_end.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_1.nx_selector_dst_address_end.nxd_ip_address.v4 = 0x02000000;

    /* add tunnel address.  */
    address_selector_1.nx_selector_src_tunnel_address.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_1.nx_selector_src_tunnel_address.nxd_ip_address.v4 = 0x02020305;
    address_selector_1.nx_selector_dst_tunnel_address.nxd_ip_version = NX_IP_VERSION_V4;
    address_selector_1.nx_selector_dst_tunnel_address.nxd_ip_address.v4 = 0x02020304;

    /* Set up TUNNEL */
    status = nx_tunnel_create(&ip_1, &tunnel_1,NX_IP_VERSION_V6,address_selector_1);

    if (status)
        error_counter++;
    /* Ensure the IP instance has been initialized.  */
    status =  nx_ip_status_check(&ip_1, NX_IP_INITIALIZE_DONE, &actual_status, NX_IP_PERIODIC_RATE);

    /* Check status...  */
    if (status != NX_SUCCESS)
    {

        error_counter++;
        test_control_return(1);
    }

    /* Create a socket.  */
    status =  nx_tcp_socket_create(&ip_1, &server_socket, "Server Socket", 
                                NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE, 100,
                                NX_NULL, thread_1_disconnect_received);
                                
    /* Check for error.  */
    if (status)
        error_counter++;

    /* Setup this thread to listen.  */
    status =  nx_tcp_server_socket_listen(&ip_1, 12, &server_socket, 5, thread_1_connect_received);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Accept a client socket connection.  */
    status =  nx_tcp_server_socket_accept(&server_socket, 5 * NX_IP_PERIODIC_RATE);

    /* Check for error.  */
    if (status)
        error_counter++;

    /* Suspend thread 0.  */
    tx_thread_suspend(&thread_0);

    /* Receive a TCP message from the socket.  */
    if(server_socket.nx_tcp_socket_receive_queue_head)
    {
        status =  nx_tcp_socket_receive(&server_socket, &packet_ptr, 5 * NX_IP_PERIODIC_RATE);

        /* Check for error.  */
        if (status)
            error_counter++;
        else
        {
            if(packet_ptr -> nx_packet_length == 0)
                error_counter++;

            memcpy(&rcv_buffer[recv_length], packet_ptr -> nx_packet_prepend_ptr, packet_ptr -> nx_packet_length);
            recv_length = packet_ptr -> nx_packet_length;

            /* Release the packet.  */
            nx_packet_release(packet_ptr);
        }
    }

    if(recv_length != 26)
        error_counter++;

    if(memcmp(rcv_buffer, (void*)MSG, recv_length))
        error_counter++;

    /* Disconnect the server socket.  */
    status =  nx_tcp_socket_disconnect(&server_socket, 5 * NX_IP_PERIODIC_RATE);

    /* Unaccept the server socket.  */
    status =  nx_tcp_server_socket_unaccept(&server_socket);

    /* Determine if the test was successful.  */
    if (error_counter)
    {

        printf("ERROR!\n");
        test_control_return(1);
    }
    else
    {

        printf("SUCCESS!\n");
        test_control_return(0);
    }

    
}


static void  thread_1_connect_received(NX_TCP_SOCKET *socket_ptr, UINT port)
{

    /* Check for the proper socket and port.  */
    if ((socket_ptr != &server_socket) || (port != 12))
        error_counter++;
    else
        connections++;
}


static void  thread_1_disconnect_received(NX_TCP_SOCKET *socket)
{

    /* Check for proper disconnected socket.  */
    if (socket != &server_socket)
        error_counter++;
    else
        disconnections++;
}

static void  thread_0_receive_notify(NX_TCP_SOCKET *client_socket)
{

    client_receives++;
}


static void  thread_1_receive_notify(NX_TCP_SOCKET *server_socket)
{

    server_receives++;
}
#else

#ifdef CTEST
VOID test_application_define(void *first_unused_memory)
#else
void    netx_tcp_tunnel_ipv4_ipv4_basic_test_application_define(void *first_unused_memory)
#endif
{

    /* Print out some test information banners.  */
    printf("NetX Test:   TUNNEL TCP IPV4_4 Basic Processing Test...................N/A\n");

    test_control_return(3);

}
#endif
