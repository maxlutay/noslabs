#include<stdio.h> //printf
#include<string.h> //memset,memcpy,strtok,strcmp
#include<stdlib.h> //for exit(0);
#include<sys/socket.h>
#include<errno.h> //For errno - the error number
#include<pthread.h>
#include<netdb.h> //getaddrinfo/freeaddrinfo
#include<arpa/inet.h> //hton*/ntoh*, inet_ntoa
#include<netinet/tcp.h>//struct tcphdr
#include<netinet/ip.h>//struct ip
#include<unistd.h>//close(fd)
#include<ifaddrs.h>//getifaddrs/freeifaddrs
#include<net/if.h>//IFF_LOOPBACK
#include<time.h>


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
    struct sockaddr_in *dest, *src;
    struct sockparam sockarg;
};


struct pseudoheader{
    uint32_t src, dest;
    uint8_t zero, protocol;
    uint16_t tcplength; 
};



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


    struct sockaddr_in * src;
    struct sockaddr_in * dest;

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
    ha_a->src = src;
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



    pthread_join(handler_thread,NULL);
    pthread_join(emitter_thread,NULL);


    printf(">End\n");
};



void* emitter(void* _args){
    struct emitter_args* ema = (struct emitter_args*)_args;

    printf(">emitter\n");
    fflush(stdout);

    char buffer[576];
    memset(buffer, 0, sizeof(buffer));

    srand(time(NULL));

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
    //iph->ip_sum = chsum((uint16_t*)iph, sizeof(struct ip));//prev checksum = 0// autocomputed



    struct tcphdr* tcph = (struct tcphdr*) (buffer + sizeof(struct ip));//20-39bytes
    tcph->source = htons(rand()); // source port
    tcph->dest = 0; // dest port, will be changing 
    tcph->seq = htonl(rand());//sequence number
    tcph->ack_seq = 0;//acknowledge sequence number
    tcph->doff = sizeof(struct tcphdr)/4;// tcp data offset in 32bit words, 1/8/4=1/32
    tcph->res1 = 0;//reserved
    //flags//todo: provide other methods
    tcph->fin = 0;
    tcph->syn = 1;
    tcph->rst = 0;
    tcph->psh = 0;
    tcph->ack = 0;
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
    if(-1 == setsockopt(sock_fd,IPPROTO_IP,IP_HDRINCL,&one,4)) printf("error emitter sockopt: %d %s", errno, strerror(errno));

    struct pseudoheader psh;
    psh.src = iph->ip_src.s_addr;
    psh.dest = iph->ip_dst.s_addr;
    psh.zero = 0;
    psh.protocol = iph->ip_p;
    psh.tcplength = htons(sizeof(struct tcphdr));//tcphlen+datalen

    uint8_t tmp[sizeof(struct pseudoheader)+sizeof(struct tcphdr)];//pseudolen+tcphdrlen
    
    memcpy(tmp,&psh,sizeof(struct pseudoheader));

    for(uint16_t p = ema->port_from; p <= ema->port_to; p++){

        tcph->dest = htons(p);
        tcph->check = 0;//skip while computing

        memcpy(tmp+sizeof(struct pseudoheader),tcph,sizeof(struct tcphdr));

        tcph->check = chsum((uint16_t*)tmp,sizeof(tmp));//!!do not convert htons

        int sendbuflen = sizeof(struct ip) + sizeof(struct tcphdr);

        int err = sendto(sock_fd, buffer, sendbuflen, 0, (struct sockaddr*)ema->dest, sizeof(struct sockaddr) );
        if(err == -1){ 
            printf("Error(%d) emitter sending packet: %s\n",errno, strerror(errno));
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


    int salen = sizeof(struct sockaddr); 
    
    while(1){
        int16_t recieved_len = recvfrom(sock_fd, buffer, 8<<16 -1 ,0, (struct sockaddr*)haa->dest, &salen);
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
        struct tcphdr* tcph = (struct tcphdr*)(buffer + iph->ip_hl * 4);//bufferaddr + tobytes(ip_headerlength) // 1/32*4=1/8

        if(iph->ip_src.s_addr == haa->dest->sin_addr.s_addr){
            if(tcph->ack == 1 && tcph->syn == 1){//open
                printf("synack :%d (open)\n", ntohs(tcph->source) );
            }else if(tcph->rst = 1){
                printf("rst :%d (closed)\n", ntohs(tcph->source) );
            }else{
                printf("unusual response  :%d \n", ntohs(tcph->source) );
            };
        };

    };

    close(sock_fd);

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

    //summ all 16bit words
    while(nbytes >1){
        sum += *(data_ptr++);
        nbytes -= sizeof(uint16_t);//2
    };

    //if odd len, sum+= lastbyte
    if(nbytes > 0) sum += *(uint8_t*)data_ptr;

    //sum = high + low: sum <= 8fff7  
    sum = (sum & 0x0000ffff) + (sum >> 16);

    //fit 16 bit result: sum <= 8fff7 + 8, sum <= 8ffff  
    sum += (sum >> 16);

    //return binarynot sum: sum <= ffff
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
        if(skt != -1) conn = connect(skt,tmp->ai_addr,tmp->ai_addrlen);//check can connect 
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




