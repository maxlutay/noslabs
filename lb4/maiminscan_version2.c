#include<stdio.h> //printf
#include<string.h> //memset,strtok,strcmp
#include<stdlib.h> //for exit(0);
#include<sys/socket.h>
#include<errno.h> //For errno - the error number
#include<pthread.h>
#include<netdb.h> //getaddrinfo
#include<arpa/inet.h>//hton/ntoh, inet_pton/inet_ntop
#include<netinet/tcp.h>//struct tcphdr
#include<netinet/ip.h>//struct ip
#include<unistd.h>//close(fd)
#include<ifaddrs.h>//getifaddrs/freeifaddrs

void help(char* message);
uint16_t chsum(uint16_t* data_ptr, int len);
struct sockaddr* resolve_hostname(char* hostname, struct addrinfo* hints_p);
struct sockaddr* get_self_addr(uint16_t family);

int main(int argc, char* argv[]){
    char* hostname;//max 253
    uint16_t from_port = 0, to_port = ~0;//default from_port=0, to_port=2^16-1=65535

    //arguments processing 
    if(argc != 2  && argc != 4 ) { help(NULL);};
    hostname = argv[1];
    if(strlen(hostname) > 253){ help("hostname length > 253"); };
    if(argc == 4){
        if(strcmp(argv[2],"-p")==0){//if '-p'
            int tmp_from = atoi(strtok(argv[3],"-"));
            if(tmp_from < 0 || tmp_from > to_port){ help("invalid port"); };

            from_port = tmp_from;

            char* tmp = strtok(NULL,"-");
            if(NULL!=strtok(NULL,"-")){ help("invalid port format"); };
            if(tmp == NULL){//if only from specified
                to_port = from_port;
          } else {
                int tmp_to = atoi(tmp);               
                if(tmp_to < 0 || tmp_to > to_port){ help("invalid port"); };
                to_port = tmp_to;
            };

            //todo: check tmpto\\tmpfrom == null
            //
        }else{ help(NULL); }; 
    };
    //end arguments processing




    printf("\n>host: %s\n>from: %d\n>to: %d\n\n", hostname, from_port, to_port);


    struct sockaddr_in src, dest;
    
    //resolve hostname
    struct addrinfo*  ainfoarr;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_TCP;
    //hints.ai_flags = 0;
    hints.ai_canonname = NULL; 
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    dest = *(struct sockaddr_in*)resolve_hostname(hostname,&hints);

    printf(">'%s' resolved to %s \n", hostname, inet_ntoa(dest.sin_addr));
    
    src = *(struct sockaddr_in*)get_self_addr(hints.ai_family);
    printf(">self resolved to %s \n\n", inet_ntoa(src.sin_addr));


        
    int emitter_sock_fd = socket(AF_INET,SOCK_RAW,IPPROTO_TCP);

    if(emitter_sock_fd == -1){
        printf("Error socket creation: %d %s\n", errno, strerror(errno));
        exit(0); 
    };


 //   int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
 //   if(sock < 1){ printf("Error socket creation: %d %s\n", errno, strerror(errno)); exit(0); };
    printf(">Socket created.\n");
    
    char buffer[2048];
    memset(buffer, 0, 2048);

    struct ip* iph = (struct ip*) buffer;//0-19bytes
    struct tcphdr* tcph = (struct tcphdr*) (buffer + sizeof(struct ip));//20-39bytes

}





void help(char* message){
    if(message != NULL) printf("\ninput error:''%s''", message);
    printf("usage:\n\tscan <hostname> [-p <port from>-<port to> | -p <port>]\n\n");
    exit(0);
}

uint16_t chsum(uint16_t* data_ptr, int len){//len in bytes
    uint32_t sum = 0;
    while(len >1){
        sum += *(data_ptr++);
        len -= 2;
    };
    
    //if odd len
    if(len > 0) sum += *(uint8_t*)data_ptr;
    
    //fit 16bit
    while(sum >> 16){ sum = (sum & 0xffff) + (sum >> 16); };
    
    return (uint16_t)(~sum);//invert
    
};


struct sockaddr* resolve_hostname(char* hostname, struct addrinfo* hints_p){

    struct addrinfo*  ainfollist;
    int err = getaddrinfo(hostname, NULL, hints_p, &ainfollist);
    

    struct addrinfo* tmp;
    for(tmp = ainfollist;;tmp = tmp->ai_next){
        if(tmp == NULL)  err = 1;
        if( err != 0 ){
            printf("error resolving '%s' : %s\n", hostname, gai_strerror(err));
            exit(0);
        };

        int skt,conn;
        skt = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
        if(skt != -1) conn = connect(skt,tmp->ai_addr,tmp->ai_addrlen);
        close(skt);
        if(conn == 0){  break; };
    };
    
    struct sockaddr* ret = malloc(sizeof(struct sockaddr*));//for freeaddrinfo
    memcpy(ret,tmp->ai_addr,sizeof(struct sockaddr*));
    freeaddrinfo(ainfollist);

    return ret;
};

struct sockaddr* get_self_addr(uint16_t family){
    struct ifaddrs* ifllist;
    if(getifaddrs(&ifllist) == -1){
        printf("error getting self address\n");
        exit(0);
    };

    struct ifaddrs* tmp;
    for(tmp = ifllist; tmp != NULL; tmp = tmp->ifa_next){
        if(tmp->ifa_addr != NULL && tmp->ifa_addr->sa_family == family) break;
    };

    struct sockaddr* ret = malloc(sizeof(struct sockaddr*));//for freeifaddrs
    memcpy(ret,tmp->ifa_addr,sizeof(struct sockaddr*));
    freeifaddrs(ifllist);

    return ret;
};
//uint16_t tcp_checksum(struct ip* iph, struct tcphdr* tcph){}


    /* 
	void * receive_state( void *ptr );
	void process_packet(unsigned char* , int);
	unsigned short csum(unsigned short * , int );
	char * hostname_to_ip(char * );
	int get_local_ip (char *);
	 
	struct in_addr dest_ip;
	 
	int main(int argc, char *argv[])
	{
        //Create a raw socket
        int s = socket (AF_INET, SOCK_RAW , IPPROTO_TCP);
        if(s < 0)
        {
            printf ("Error creating socket. Error number : %d . Error message : %s \n" , errno ,strerror(errno));
            exit(0);
        }
        else
        {
            printf("Socket created.\n");
        }

        //Datagram to represent the packet
        char datagram[4096];   


        //IP header
        struct iphdr* iph = (struct iphdr *) datagram;

        //TCP header
        struct tcphdr* tcph = (struct tcphdr *) (datagram + sizeof (struct ip));//tcp header begins at datagram start + ip header length

        struct sockaddr_in  dest;

        char* target = argv[1];

        if(argc < 2)
        {
            printf("Please specify a hostname \n");
            exit(1);
        }

        if( inet_addr( target ) != -1)
        {
            dest_ip.s_addr = inet_addr( target );
        }
        else
        {
            char *ip = hostname_to_ip(target);
            if(ip != NULL)
            {
                printf("%s resolved to %s \n" , target , ip);
                //Convert domain name to IP
                dest_ip.s_addr = inet_addr( hostname_to_ip(target) );
            }
            else
            {
                printf("Unable to resolve hostname : %s" , target);
                exit(1);
            }
        }

        int source_port = 43591;
        char source_ip[20];
        get_local_ip( source_ip );

        printf("Local source IP is %s \n" , source_ip);

        memset (datagram, 0, 4096); // zero out the buffer 

        //Fill in the IP Header
        (*iph) = struct iphdr{
            .ihl = 5,
            .version = 4,
            .tos = 0,
            .tot_len = sizeof (struct ip) + sizeof (struct tcphdr),
            .id = htons (54321), //Id of this packet
            .frag_off = htons(16384),
            .ttl = 64,
            .protocol = IPPROTO_TCP,
            .check = 0, //kernel autocompute   
            .saddr = inet_addr ( source_ip ),    //Spoof the source ip address
            .daddr = dest_ip.s_addr
        };

        //TCP Header
        (*tcph) = {
            .source = htons ( source_port ),
            .dest = htons (80),//will be changing
            .seq = htonl(1105024978),//whatever
            .ack_seq = 0,
            .doff = sizeof(struct tcphdr) / 32, //tcp header size in 32bitwords
            .fin=1,
            .syn=0,
            .rst=0,
            .psh=0,
            .ack=1,
            .urg=0,
            .window = htons ( 14600 ),  // maximum allowed window size
            .check = 0,//while computing keep check=0
            .urg_ptr = 0,//not matters cause URG flag=0
        };
        //IP_HDRINCL to tell the kernel that headers are included in the packet
        int one = 1;
        const int *val = &one;

        if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
        {
            printf ("Error setting IP_HDRINCL. Error number : %d . Error message : %s \n" , errno, strerror(errno));
            exit(0);
        }

        printf("Starting sniffer thread...\n");
        char *message1 = "Thread 1";
        int  iret1;
        pthread_t sniffer_thread;

        if( pthread_create( &sniffer_thread , NULL ,  receive_state , (void*) message1) < 0) {
            printf ("Could not create sniffer thread. Error number : %d . Error message : %s \n" ,errno , strerror(errno));
            exit(0);
        };

        printf("Starting to send fin/ack packets\n");

        int port;
        dest.sin_family = AF_INET;
        dest.sin_addr.s_addr = dest_ip.s_addr;

        int fromrange = atoi(argv[2]), torange = atoi(argv[3]);

        printf("\nfrom :%d to :%d \n", fromrange, torange);

        for(port = fromrange; port < torange; port++) {
            tcph->dest = htons(port);
            tcph->check = 0;
            //tcph->check =  //compute

            //Send the packet
            if ( sendto (s, datagram , sizeof(struct iphdr) + sizeof(struct tcphdr) , 0 , (struct sockaddr *) &dest, sizeof (dest)) < 0) {
                printf ("Error sending tcp fin/ack packet. Error number : %d . Error message : %s \n" ,errno , strerror(errno));
                exit(0);
            }
        }

        pthread_join( sniffer_thread , NULL);
        //printf("%d" , iret1);

        return 0;
}
 
//  Method to sniff incoming packets and look for Ack replies

void * receive_state( void *ptr )
{
    //Start the sniffer thing
    start_sniffer();
}
 
int start_sniffer()
{
    int sock_raw;
     
    int saddr_size , data_size;
    struct sockaddr saddr;
     
    unsigned char *buffer = (unsigned char *)malloc(65536); //maximum possible size
     
    printf("Sniffer initialising...\n");
    fflush(stdout);
     
    //Create a raw socket that  will sniff
    sock_raw = socket(AF_INET , SOCK_RAW , IPPROTO_TCP);
    
    //struct timeval tv;
    //tv.tv_sec = 60;
    //setsockopt(sock_raw,SOL_SOCKET,SO_RCVTIMEO, &tv, sizeof(struct timeval) );    
 
    
    if(sock_raw < 0)
    {
        printf("Socket Error\n");
        fflush(stdout);
        return 1;
    }
     
    saddr_size = sizeof saddr;
     
    while(1)
    {
        //Receive a packet
        printf("\nlistening\n");
        data_size = recvfrom(sock_raw , buffer , 65536 , 0 , &saddr , &saddr_size);
        if(data_size <0 )
        {
            if(errno == EWOULDBLOCK){break;};
            printf("Recvfrom error , failed to get packets\n");
            fflush(stdout);
            return 1;
        }
         
        //Now process the packet
        process_packet(buffer , data_size);
    }
    close(sock_raw);
    printf("Sniffer finished.\n");
    fflush(stdout);
    return 0;
}
 
void process_packet(unsigned char* buffer, int size)
{
    //Get the IP Header part of this packet
    struct iphdr *iph = (struct iphdr*)buffer;
    struct sockaddr_in source,dest;
    unsigned short iphdrlen;
     printf("\nproc\n");
    if(iph->protocol == 6)//tcp
    {
        struct iphdr *iph = (struct iphdr *)buffer;
        iphdrlen = iph->ihl*4;
     
        struct tcphdr *tcph=(struct tcphdr*)(buffer + iphdrlen);
             
        memset(&source, 0, sizeof(source));
        source.sin_addr.s_addr = iph->saddr;
     
        memset(&dest, 0, sizeof(dest));
        dest.sin_addr.s_addr = iph->daddr;
         
        if(tcph->rst == 1 && source.sin_addr.s_addr == dest_ip.s_addr )
        {
            printf("+ :%d on \n" , ntohs(tcph->source));
            fflush(stdout);
        }
    }
}
 
// Checksum - IP and TCP

unsigned short csum(unsigned short *ptr,int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum+=oddbyte;
    }

    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;

    return(answer);
}
 
// Get ip from domain name
char* hostname_to_ip(char * hostname)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return NULL;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        return inet_ntoa(*addr_list[i]) ;
    }

    return NULL;
}
 
// Get source IP of system , like 192.168.0.6 or 192.168.1.2
 
int get_local_ip ( char * buffer)
{
    int sock = socket ( AF_INET, SOCK_DGRAM, 0);

    const char* kGoogleDnsIp = "8.8.8.8";
    int dns_port = 53;

    struct sockaddr_in serv;

    memset( &serv, 0, sizeof(serv) );
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
    serv.sin_port = htons( dns_port );

    int err = connect( sock , (const struct sockaddr*) &serv , sizeof(serv) );

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    err = getsockname(sock, (struct sockaddr*) &name, &namelen);

    const char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);

    close(sock);
}	

*/
