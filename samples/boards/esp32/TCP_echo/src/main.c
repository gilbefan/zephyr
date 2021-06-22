#include<zephyr.h>
#include<esp_wifi.h>


#include<stdio.h>
//#include<stdlib.h>
//include<string.h>
//include<sys/types.h>
#include<net/socket.h>

#include <logging/log.h>

LOG_MODULE_REGISTER(esp32_wifi_sta, LOG_LEVEL_DBG);

static struct net_mgmt_event_callback dhcp_cb;
static struct k_work_delayable tcp_timer;

static void tcp_start() {
    //TCP CONNECTION

    int network_socket;
    network_socket=zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (network_socket < 0) {
        LOG_ERR("Failed to create TCP socket: %d",
			errno);
        return;
    }
    //socket created
 
    struct sockaddr_in server_address;
    server_address.sin_family=AF_INET;
    server_address.sin_port=htons(1080);
    zsock_inet_pton(AF_INET, "10.18.208.250",
			  &server_address.sin_addr);
 
    int connect_socket=zsock_connect(network_socket,(struct sockaddr*)&server_address,sizeof(struct sockaddr_in));
    if(connect_socket==-1)
    {
        LOG_ERR("Error connecting socket: %d", errno);
        return;
    }
    //socket connected to the server
 
    char ch[1024];
     
    //recieve data from server
    while(1){
        zsock_recv(network_socket, &ch, sizeof(ch), 0);
        printf("The recieved data from the server is:\n%s",ch);
    }
     
    zsock_close(network_socket);
}

static void handler_cb(struct net_mgmt_event_callback *cb,
		    uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event != NET_EVENT_IPV4_DHCP_BOUND) {
		return;
	}

	char buf[NET_IPV4_ADDR_LEN];

	LOG_INF("Your address: %s",
		log_strdup(net_addr_ntop(AF_INET,
				   &iface->config.dhcpv4.requested_ip,
				   buf, sizeof(buf))));
	LOG_INF("Lease time: %u seconds",
			iface->config.dhcpv4.lease_time);
	LOG_INF("Subnet: %s",
		log_strdup(net_addr_ntop(AF_INET,
					&iface->config.ip.ipv4->netmask,
					buf, sizeof(buf))));
	LOG_INF("Router: %s",
		log_strdup(net_addr_ntop(AF_INET,
						&iface->config.ip.ipv4->gw,
						buf, sizeof(buf))));
    
    k_work_init_delayable(&tcp_timer, tcp_start);
	k_work_reschedule(&tcp_timer, K_NO_WAIT);
}

void main()
{
    //WIFI CONNECTION
    struct net_if *iface;

	net_mgmt_init_event_callback(&dhcp_cb, handler_cb,
				     NET_EVENT_IPV4_DHCP_BOUND);

	net_mgmt_add_event_callback(&dhcp_cb);

	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("wifi interface not available");
		return;
	}

	net_dhcpv4_start(iface);

	if (!IS_ENABLED(CONFIG_ESP32_WIFI_STA_AUTO)) {
		wifi_config_t wifi_config = {
			.sta = {
				.ssid = CONFIG_ESP32_WIFI_SSID,
				.password = CONFIG_ESP32_WIFI_PASSWORD,
			},
		};

		esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_STA);

		ret |= esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
		ret |= esp_wifi_connect();
		if (ret != ESP_OK) {
			LOG_ERR("connection failed");
		}
	}
}   