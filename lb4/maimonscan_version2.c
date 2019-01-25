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
#include<net/if.h>

void help(char* message);
uint16_t chsum(uint16_t* data_ptr, int len);
struct sockaddr* resolve_hostname(char* hostname, struct addrinfo* hints_p);
struct sockaddr* get_self_addr(uint16_t family);
void* handler(void* _args);
void* emitter(void* _args);


struct sockparam { int family, socktype, protocol; };

struct emitter_args {
    struct sockaddr_in * src, * dest;
    uint16_t port_from, port_to;
    struct sockparam sockarg;
};


struct handler_args {
    struct sockaddr_in *dest;
    struct sockparam sockarg;
};


struct pseudoheader{
    uint32_t src, dest;
    uint8_t zero, protocol;
    uint16_t tcplength; 
};


pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

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
        }else{ help("invalid flag"); }; 
    };
    //end arguments processing




    printf("\n>host: %s\n>from: %d\n>to: %d\n\n", hostname, from_port, to_port);


    struct sockaddr_in * src, * dest;
    
    //resolve hostname
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;
    hints.ai_canonname = NULL; 
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    dest = (struct sockaddr_in*)resolve_hostname(hostname,&hints);

    printf(">'%s' resolved to %s\n", hostname, inet_ntoa(dest->sin_addr) );
    
    src = (struct sockaddr_in*)get_self_addr(hints.ai_family);
    printf(">self %s \n\n", inet_ntoa(src->sin_addr) );
    
    printf(">Starting threads...\n");

    pthread_t handler_thread, emitter_thread;
    
    int th_err = 0;




    struct handler_args* ha_a = calloc(1,sizeof(struct handler_args));
    ha_a->dest = dest;
    ha_a->sockarg.family = hints.ai_family;
    ha_a->sockarg.socktype = hints.ai_socktype;
    ha_a->sockarg.protocol = hints.ai_protocol;

    th_err += pthread_create( &handler_thread, NULL,  handler, (void*) ha_a);



    struct emitter_args* em_a = calloc(1,sizeof(struct emitter_args));
    em_a->src = src;
    em_a->dest = dest;
    em_a->port_to = to_port;
    em_a->port_from = from_port;
    em_a->sockarg = ha_a->sockarg;

    th_err += pthread_create( &emitter_thread, NULL, emitter, (void*)em_a );

   
    if( th_err != 0){
        printf("Error threads creation\n");
        exit(EXIT_FAILURE);
    };

   

    pthread_join(&handler_thread,NULL);
    pthread_join(&emitter_thread,NULL);


    printf(">End\n");
};



void* emitter(void* _args){
    struct emitter_args* ema = (struct emitter_args*)_args;

    printf(">emitter\n");
    fflush(stdout);


    char buffer[576];
    memset(buffer, 0, sizeof(buffer));

    struct ip* iph = (struct ip*) buffer;//0-19bytes
    iph->ip_hl = sizeof(struct ip)/4;//ip header length in 32bitwords
    iph->ip_v = 4;//version
    iph->ip_tos = 0;//type of service
    iph->ip_len = htons(sizeof(struct ip)+sizeof(struct tcphdr));//iph+data length bytes, in this case iph+tcph
    iph->ip_id = htons(rand());//id of packet
    iph->ip_off = 0;//offset(in packet)
    iph->ip_ttl = 64;//time to live
    iph->ip_p = ema->sockarg.protocol;//encapsulated protocol type
    iph->ip_src = ema->src->sin_addr;//source ip address
    iph->ip_dst = ema->dest->sin_addr;//dest ip address
    iph->ip_sum = 0;
    //iph->ip_sum = chsum((uint16_t*)iph, sizeof(struct ip));//prev checksum = 0// autocompute

   

    struct tcphdr* tcph = (struct tcphdr*) (buffer + sizeof(struct ip));//20-39bytes
    tcph->source = htons(rand()); // source port
    tcph->dest = 0; // dest port, will be changing 
    tcph->seq = htonl(rand());//sequence number
    tcph->ack_seq = 0;//acknowledge sequence number
    tcph->doff = sizeof(struct tcphdr)/4;// tcp data offset in 32bit words, 1/8/4=1/32
    tcph->res1 = 0;//reserved
    //flags
    tcph->fin = 1;//maimon
    tcph->syn = 0;
    tcph->rst = 0;
    tcph->psh = 0;
    tcph->ack = 1;//maimon
    tcph->urg = 0;
    //
    tcph->window = htons(14600);
    tcph->check = 0;//should be manually computed, else dropped
    tcph->urg_ptr = 0;// ignored cause tcph->urg


    printf(">source port %d\n", ntohs(tcph->source));

    int sock_fd = socket(ema->sockarg.family,ema->sockarg.socktype,ema->sockarg.protocol);

    if(sock_fd == -1){
        printf("Error emitter socket creation: %d %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE); 
    };

    int one=1;
    if(-1 == setsockopt(sock_fd,IPPROTO_IP,IP_HDRINCL,&one,4)) printf("errr %d %s", errno, strerror(errno));



    pthread_mutex_lock(&m);

    for(uint16_t p = ema->port_from; p <= ema->port_to; p++){
        struct pseudoheader psh;
        psh.src = iph->ip_src.s_addr;
        psh.dest = iph->ip_dst.s_addr;
        psh.zero = 0;
        psh.protocol = iph->ip_p;
        psh.tcplength = htons(sizeof(struct tcphdr));//tcphlen+datalen

        tcph->dest = htons(p);
        tcph->check = 0;

        uint8_t tmp[sizeof(struct pseudoheader)+sizeof(struct tcphdr)];//pseudolen+tcphdrlen
        memcpy(tmp,&psh,sizeof(struct pseudoheader));
        memcpy(tmp+sizeof(struct pseudoheader),tcph,sizeof(struct tcphdr));

        tcph->check = chsum((uint16_t*)tmp,sizeof(tmp));//!!do not convert htons


        int sendbuflen = sizeof(struct ip) + sizeof(struct tcphdr);

        int err = sendto(sock_fd, buffer, sendbuflen, 0, (struct sockaddr*)ema->dest, sizeof(struct sockaddr) );
        /*debug
        printf("2*\n");
        for(int i = 0; i < sendbuflen; i++){
                printf("%02x", (uint8_t)buffer[i]);

                printf("_");
                if(i%4 == 3) printf("\n");
        }        
        printf("*\n");
        */
        if(err == -1){ 
            printf("Error(%d) sending packet: %s\n",errno, strerror(errno));
            close(sock_fd);
            exit(EXIT_FAILURE);
        };

};//endfor

close(sock_fd);


printf(">emitter fin\n");
fflush(stdout);
}

void* handler(void* _args){
    printf("\n>handler\n");
    fflush(stdout);

    pthread_mutex_trylock(&m);




    struct handler_args* haa = (struct handler_args*)_args;
    int sock_fd = socket(haa->sockarg.family,haa->sockarg.socktype,haa->sockarg.protocol);
    if(sock_fd == -1){
        printf("Error handler socket creation: %d %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE); 
    };

    uint8_t* buffer = malloc((uint16_t)~0);//max 65535 bytes
    
    struct timeval tv;
    tv.tv_sec = 10;
    setsockopt(sock_fd,SOL_SOCKET,SO_RCVTIMEO, &tv, sizeof(struct timeval) );    
    
    pthread_mutex_destroy(&m);
    while(1){
        int salen = sizeof(struct sockaddr); 
        int16_t recieved_len = recvfrom(sock_fd, buffer, 8<<16,0, (struct sockaddr*)haa->dest, &salen);
        if(recieved_len <= 0){
           if(errno == EWOULDBLOCK){
                printf(">Socket timeout\n");
                break;
           };
           printf("recvfrom error(%d) : %s\n", errno, strerror(errno));
           fflush(stdout);
           close(sock_fd);
           exit(EXIT_FAILURE);
        };
        struct ip* iph = (struct ip*) buffer;
        struct tcphdr* tcph = (struct tcphdr*)(buffer+iph->ip_hl);
        printf("+ %d on\n", ntohs(tcph->source) );

    };



printf(">handler fin\n");
fflush(stdout);
}





void help(char* message){
    if(message != NULL) printf("\ninput error:''%s''", message);
    printf("usage:\n\tscan <hostname> [-p <port from>-<port to> | -p <port>]\n\n");
    if(message != NULL) exit(EXIT_FAILURE);
    exit(EXIT_SUCCESS);
}

uint16_t chsum(uint16_t* data_ptr, int nbytes){//len in bytes
    uint32_t sum = 0;
    while(nbytes >1){
        sum += *(data_ptr++);
        nbytes -= sizeof(uint16_t);//2
    };

    //if odd len
    if(nbytes > 0) sum += *(uint8_t*)data_ptr;
   
    //fit 16bit
    sum = (sum & 0x0000ffff) + (sum >> 16); 
    sum += (sum >> 16);

    return ~sum;
};


struct sockaddr* resolve_hostname(char* hostname, struct addrinfo* hints_p){

    struct addrinfo*  ainfollist;
    int err = getaddrinfo(hostname, NULL, hints_p, &ainfollist);
    

    struct addrinfo* tmp;
    for(tmp = ainfollist;;tmp = tmp->ai_next){
        if(tmp == NULL)  err = 1;
        if( err != 0 ){
            printf("error resolving '%s' : %s\n", hostname, gai_strerror(err));
            exit(EXIT_FAILURE);
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
        exit(EXIT_FAILURE);
    };

    struct ifaddrs* tmp;
    for(tmp = ifllist; tmp != NULL; tmp = tmp->ifa_next){
        if(tmp->ifa_addr == NULL) continue; 
        if(tmp->ifa_addr->sa_family != family) continue;
        if( ((tmp->ifa_flags & IFF_LOOPBACK) ^ IFF_LOOPBACK) != 0 ) break;//if not loopback, save tmp
    };

    struct sockaddr* ret = malloc(sizeof(struct sockaddr*));//for freeifaddrs
    memcpy(ret,tmp->ifa_addr,sizeof(struct sockaddr*));
    freeifaddrs(ifllist);

    return ret;
};




    /* 
        printf("Starting sniffer thread...\n");

        printf("Starting to send fin/ack packets\n");

 
//  Method to sniff incoming packets and look for Ack replies

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
