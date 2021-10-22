#define _GNU_SOURCE
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#define MY_UDP_PORT 5000

int main(void)
{
	const int on=1, off=0;
	int result;
	struct sockaddr_in6 sin6;
	int soc;

	soc = socket(AF_INET6, SOCK_DGRAM, 0);
	setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	setsockopt(soc, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on));
	setsockopt(soc, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on));
	setsockopt(soc, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off));
	memset(&sin6, '\0', sizeof(sin6));
	sin6.sin6_family = AF_INET6;
	sin6.sin6_port = htons(MY_UDP_PORT);
	//sin6.sin6_addr =  in6addr_any;
	result = bind(soc, (struct sockaddr*)&sin6, sizeof(sin6));

	printf("bind result %d port %d\n", result, MY_UDP_PORT);
	if(result < 0){
		printf("error %s\n", strerror(errno));
	}

	do{
		int bytes_received;
		struct sockaddr_in6 from;
		struct iovec iovec[1];
		struct msghdr msg;
		char msg_control[1024];
		char udp_packet[1500];

		iovec[0].iov_base = udp_packet;
		iovec[0].iov_len = sizeof(udp_packet);
		msg.msg_name = &from;
		msg.msg_namelen = sizeof(from);
		msg.msg_iov = iovec;
		msg.msg_iovlen = sizeof(iovec) / sizeof(*iovec);
		msg.msg_control = msg_control;
		msg.msg_controllen = sizeof(msg_control);
		msg.msg_flags = 0;
		printf("ready to recvmsg\n");
		bytes_received = recvmsg(soc, &msg, 0);
		printf("recieved %d bytes\n", bytes_received);

		struct in_pktinfo in_pktinfo;
		struct in6_pktinfo in6_pktinfo;
		int have_in_pktinfo = 0;
		int have_in6_pktinfo = 0;
		struct cmsghdr* cmsg;

		for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != 0; cmsg = CMSG_NXTHDR(&msg, cmsg))
		{
			printf("asdf\n");
			if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_PKTINFO)
			{
				in_pktinfo = *(struct in_pktinfo*)CMSG_DATA(cmsg);
				have_in_pktinfo = 1;
				have_in6_pktinfo = 0;
				printf("recieved ipv4\n");
			}
			if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_PKTINFO)
			{
				in6_pktinfo = *(struct in6_pktinfo*)CMSG_DATA(cmsg);
				have_in6_pktinfo = 1;
				have_in_pktinfo = 0;
				printf("recieved ipv6\n");
			}
		}
		//printf("iovlen = %zu \n", msg.msg_iov->iov_len );
		int i;
		for(i = 0; i < bytes_received; i++)
		{
			printf("%02hhx ", (unsigned char)udp_packet[i]);
		}
		printf("\n");

		int cmsg_space;
		int ret;

		iovec[0].iov_base = udp_packet;
		iovec[0].iov_len = bytes_received;
		msg.msg_name = &from;
		msg.msg_namelen = sizeof(from);
		msg.msg_iov = iovec;
		msg.msg_iovlen = sizeof(iovec) / sizeof(*iovec);
		msg.msg_control = msg_control;
		msg.msg_controllen = sizeof(msg_control);
		msg.msg_flags = 0;
		cmsg_space = 0;
		cmsg = CMSG_FIRSTHDR(&msg);
		if (have_in6_pktinfo)
		{
			printf("sent via ipv6\n");
			cmsg->cmsg_level = IPPROTO_IPV6;
			cmsg->cmsg_type = IPV6_PKTINFO;
			cmsg->cmsg_len = CMSG_LEN(sizeof(in6_pktinfo));
			*(struct in6_pktinfo*)CMSG_DATA(cmsg) = in6_pktinfo;
			cmsg_space += CMSG_SPACE(sizeof(in6_pktinfo));
		}
		if (have_in_pktinfo)
		{
			printf("sent via ipv4\n");
			cmsg->cmsg_level = IPPROTO_IP;
			cmsg->cmsg_type = IP_PKTINFO;
			cmsg->cmsg_len = CMSG_LEN(sizeof(in_pktinfo));
			*(struct in_pktinfo*)CMSG_DATA(cmsg) = in_pktinfo;
			cmsg_space += CMSG_SPACE(sizeof(in_pktinfo));
			//in_pktinfo.ipi_spec_dst.sin_port = htons(5002);
		}
		msg.msg_controllen = cmsg_space;
		ret = sendmsg(soc, &msg, 0);
		printf("sent %d bytes\n", ret);
		if(ret < 0){
			printf("error %s\n", strerror(errno));
		}
	}while(1);

	return 0;

}
