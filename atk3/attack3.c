/*

Austin Brodeur 30007952

This code is a modified version of the code found at https://www.binarytides.com/raw-sockets-c-code-linux/

*/


#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <unistd.h>

struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t tcp_length;
};

unsigned short csum(unsigned short *ptr, int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;

    sum = 0;

    while(nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char*)&oddbyte)=*(u_char*)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum = sum + (sum >> 16);
    answer = (short)~sum;

    return (answer);
}

int main(int argc, char *argv[])
{
    int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);

    if (s == -1)
    {
        perror("Failed to create socket");
        exit(1);
    }

    if (argc != 5)
    {
        printf("usage: source_address source_port dest_address dest_port");
        exit(1);
    }

    // datagram to represent the packet
    char datagram[4096], source_ip[32], *data, *pseudogram, *dest_adr, *source_adr;
    int source_port, dest_port;

    // assign port and out adr from command line args
    source_adr = argv[1];
    source_port = atoi(argv[2]);
    dest_adr = argv[3];
    dest_port = atoi(argv[4]);

    // zero out the packet buffer
    memset (datagram, 0, 4096);

    // IP header
    struct iphdr *iph = (struct iphdr *) datagram;

    // TCP header
    struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
    struct sockaddr_in sin;
    struct pseudo_header psh;

    // Data content
    data = datagram + sizeof(struct iphdr) + sizeof(struct tcphdr);
    strcpy(data, "fake reply :)");

    // Address resolution
    strcpy(source_ip, source_adr);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(source_port);
    sin.sin_addr.s_addr = inet_addr(dest_adr);

    // Fill IP header
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof (struct iphdr) + sizeof (struct tcphdr) + strlen(data);
    iph->id = htonl (0);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; // for checksum
    iph->saddr = inet_addr (source_ip);
    iph->daddr = sin.sin_addr.s_addr;

    // IP Checksum
    iph->check = csum ((unsigned short *) datagram, iph->tot_len);

    //TCP header
    tcph->source = htons(source_port);
    tcph->dest = htons(dest_port);
    tcph->seq = 1;
    tcph->ack_seq = 0;
    tcph->doff = 5; // tcp header size
    tcph->fin = 0;
    tcph->syn = 0;
    tcph->rst = 0; // reset the connection
    tcph->psh = 1;
    tcph->ack = 1;
    tcph->urg = 0;
    tcph->window = htons(65280);
    tcph->check = 0;
    tcph->urg_ptr = 0;

    // TCP checksum
    psh.source_address = inet_addr(source_ip);
    psh.dest_address = sin.sin_addr.s_addr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + strlen(data));

    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + strlen(data);
    pseudogram = malloc(psize);

    memcpy(pseudogram, (char*) &psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr) + strlen(data));

    tcph->check = csum((unsigned short*) pseudogram, psize);

    int one = 1;
    const int *val = &one;

    if (setsockopt(s, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0)
    {
        perror("Error setting IP_HDRINCL");
        exit(0);
    }


    // while (1)
    // {
    if (sendto(s, datagram, iph->tot_len, 0, (struct sockaddr *) &sin, sizeof(sin)) < 0)
    {
        perror("sento failed");
    }
    else
    {
        printf("Packet sent. Length: %d \n", iph->tot_len);
    }
    //}

    return 0;
}

