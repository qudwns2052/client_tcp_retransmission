#include "include.h"

static uint8_t global_client_ip[4];
static uint8_t global_server_ip[4];
static uint16_t global_connection = 0;
static uint16_t global_sport;
static uint32_t global_Seq_number;
static uint32_t global_Ack_number;
static unsigned char global_packet[10000];
static unsigned char global_sub_packet[10000];
static int global_ret = 0;
static int global_sub_ret = 0;
static int global_NF_VERDICT = 0;


void dump(unsigned char* buf, int size) {
    int i;
    for (i = 0; i < size; i++) {
        if (i % 16 == 0)
            printf("\n");
        printf("%02x ", buf[i]);
    }
}


/* returns packet id */
static u_int32_t print_pkt (struct nfq_data *tb)
{
    int id = 0;
    struct nfqnl_msg_packet_hdr *ph;
    struct nfqnl_msg_packet_hw *hwph;
    u_int32_t mark,ifi;
    int ret;
    unsigned char *data;

    ph = nfq_get_msg_packet_hdr(tb);
    if (ph)
    {
        id = ntohl(ph->packet_id);
    }

    ret = nfq_get_payload(tb, &data);

    //*****************************************************************//

    Ip * data_ip_header = (Ip *)data;
    int ip_size = (data_ip_header->VHL & 0x0F) * 4;
    Tcp * data_tcp_header = (Tcp *)(data + ip_size);
    uint8_t flag = data_tcp_header->flag & 0x3f;


    //---------Is ACK packet?-------------------------------------
    if(flag == 0x10 && global_connection == 0)
    {
        printf("Send ACK packet\n");
        printf("Get info\n");

        global_Seq_number = ntohl(data_tcp_header->seq);
        global_Ack_number = ntohl(data_tcp_header->ack);
        global_sport = ntohs(data_tcp_header->s_port);
        memcpy(global_server_ip, data_ip_header->d_ip, 4);

        memcpy(global_packet, data, ret);
        global_ret = ret;
        global_connection = 1;
        global_NF_VERDICT = 1;
    }

    //---------Is Encapsulation packet?-------------------------------------

    else if(ntohs(data_tcp_header->s_port) == 0x0050 && ntohs(data_tcp_header->win_size) == 0x1212 && flag == 0x18 && global_connection == 1)
    {
        // ...
    }

    //-------------------------------------------------------------


    //---------Is Fragmentation packet?--------------------------------------

    else if(ntohs(data_tcp_header->s_port) == 0x0050 && ntohs(data_tcp_header->win_size) == 0x1234 && flag == 0x18 && global_connection == 1)
    {
        // ...
    }
    else if(ntohs(data_tcp_header->s_port) == 0x0050 && ntohs(data_tcp_header->win_size) == 0x1235 && flag == 0x18 && global_connection == 1)
    {
        // ...
    }
    //-------------------------------------------------------------

    //----------Let's Encapsulation--------------------------------
    else if(global_connection == 1)
    {
        // ...
    }
    else
    {
        memcpy(global_packet, data, ret);
        global_ret = ret;
        global_NF_VERDICT = 1;
    }

    //----------------------------------------------------------





    //*****************************************************************//
//    fputc('\n', stdout);

    return id;
}


static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
              struct nfq_data *nfa, void *data)
{
    u_int32_t id = print_pkt(nfa);
    //    printf("entering callback\n");
    return nfq_set_verdict(qh, id, global_NF_VERDICT, global_ret, global_packet);
}

int main(int argc, char **argv)
{
    //**************************************


    char * dev = "eth1";
    GET_my_ip(dev, global_client_ip);


    //**************************************
    struct nfq_handle *h;
    struct nfq_q_handle *qh;
    struct nfnl_handle *nh;
    int fd;
    int rv;
    char buf[4096] __attribute__ ((aligned));


    printf("opening library handle\n");
    h = nfq_open();
    if (!h) {
        fprintf(stderr, "error during nfq_open()\n");
        exit(1);
    }

    printf("unbinding existing nf_queue handler for AF_INET (if any)\n");
    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_unbind_pf()\n");
        exit(1);
    }

    printf("binding nfnetlink_queue as nf_queue handler for AF_INET\n");
    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "error during nfq_bind_pf()\n");
        exit(1);
    }

    printf("binding this socket to queue '0'\n");
    qh = nfq_create_queue(h,  0, &cb, NULL);
    if (!qh) {
        fprintf(stderr, "error during nfq_create_queue()\n");
        exit(1);
    }

    printf("setting copy_packet mode\n");
    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "can't set packet_copy mode\n");
        exit(1);
    }

    fd = nfq_fd(h);

    for (;;) {
        if ((rv = recv(fd, buf, sizeof(buf), 0)) >= 0)
        {
            //            printf("pkt received\n");
            nfq_handle_packet(h, buf, rv);
            continue;
        }
        /* if your application is too slow to digest the packets that
         * are sent from kernel-space, the socket buffer that we use
         * to enqueue packets may fill up returning ENOBUFS. Depending
         * on your application, this error may be ignored. nfq_nlmsg_verdict_putPlease, see
         * the doxygen documentation of this library on how to improve
         * this situation.
         */
        if (rv < 0 && errno == ENOBUFS) {
            printf("losing packets!\n");
            continue;
        }
        perror("recv failed");
        break;
    }

    printf("unbinding from queue 0\n");
    nfq_destroy_queue(qh);

#ifdef INSANE
    /* normally, applications SHOULD NOT issue this command, since
     * it detaches other programs/sockets from AF_INET, too ! */
    printf("unbinding from AF_INET\n");
    nfq_unbind_pf(h, AF_INET);
#endif

    printf("closing library handle\n");
    nfq_close(h);
    exit(0);
}
