/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "os/mynewt.h"
#include <bsp/bsp.h>

#include <hal/hal_gpio.h>
#include <hal/hal_flash.h>
#include <console/console.h>

#include <config/config.h>
#include <hal/hal_system.h>


#include <bootutil/image.h>
#include <bootutil/bootutil.h>

#include <shell/shell.h>
#include <mn_socket/mn_socket.h>
#include <inet_def_service/inet_def_service.h>

#include <assert.h>
#include <string.h>
#include <id/id.h>

/*If Open Interconnect Consortium is supported (mainly used for IoT)*/
#if MYNEWT_VAL(BUILD_WITH_OIC)

#include <oic/oc_api.h> 
#include <cborattr/cborattr.h>

#endif


#ifdef ARCH_sim

#include <mcu/mcu_sim.h>

#endif

#define SHELL_COMMAND "connect"
#define SERVER_IP_ADDRESS "192.168.10.1"
#define PORT_NUMBER_SOCKET_1 "80"
#define PORT_NUMBER_SOCKET_2 "81"

static int net_cli(int argc, char **argv); /*Network Command Line Interface*/

struct shell_cmd net_test_cmd = /*Shell commands struct*/
{
    .sc_cmd = "net",
    .sc_cmd_func = net_cli
};

/*Create 2 sockets for testing*/
static struct mn_socket *net_test_socket;
static struct mn_socket *net_test_socket2;

#if MYNEWT_VAL(BUILD_WITH_OIC)

static void omgr_app_init(void);
static const oc_handler_t omgr_oc_handler = 
{
    .init = omgr_app_init,
};

#endif

/*Functions to test connectivity*/
static void net_test_readable(void *arg, int err)
{
    console_printf("net_test_readable %x - %d\n", (int)arg, err);
}

static void net_test_writable(void *arg, int err)
{
    console_printf("net_test_writable %x - %d\n", (int)arg, err);
}

static const union mn_socket_cb net_test_cbs = 
{
    .socket.readable = net_test_readable,
    .socket.writable = net_test_writable
};

static int net_test_newconn(void *arg, struct mn_socket *new)
{
    console_printf("net_test_newconn %x - %x\n", (int)arg, (int)new);
    mn_socket_set_cbs(new, NULL, &net_test_cbs);
    net_test_socket2 = new;

    return 0;
}

static const union mn_socket_cb net_listen_cbs =  
{
    .listen.newconn = net_test_newconn,
};

static int net_cli(int argc, char **argv) /*Network Command Line Interface*/
{
    int rc;
    struct mn_sockaddr_in sin;
    struct mn_sockaddr_in *sinp;
    uint16_t port;
    uint32_t addr;
    char *eptr;
    struct os_mbuf *m;

    if (!strcmp(SHELL_COMMAND, "udp")) 
	{
        rc = mn_socket(&net_test_socket, MN_PF_INET, MN_SOCK_DGRAM, 0); /*Create UDP IPv4 socket*/
        console_printf("mn_socket(UDP) = %d %x\n", rc, (int)net_test_socket);
    } 
	else if (!strcmp(SHELL_COMMAND, "tcp")) 
	{
        rc = mn_socket(&net_test_socket, MN_PF_INET, MN_SOCK_STREAM, 0);  /*Create TCP IPv4 socket*/
        console_printf("mn_socket(TCP) = %d %x\n", rc, (int)net_test_socket);
    } 
	else if (!strcmp(SHELL_COMMAND, "connect") || !strcmp(SHELL_COMMAND, "bind"))  /*Connect or bind*/
	{
        if (mn_inet_pton(MN_AF_INET, SERVER_IP_ADDRESS , &addr) != 1) /*Convert IPv4 and IPv6 addresses from text to binary form (X.X.X.X) (stored in &addr)*/
		{
            console_printf("Invalid address %s\n", SERVER_IP_ADDRESS );
            return 0;
        }

        port = strtoul(PORT_NUMBER_SOCKET_1 , &eptr, 0); /*Converts string argument to unsigned integer to use as port*/
        if (*eptr != '\0') 
		{
            console_printf("Invalid port %s\n", PORT_NUMBER_SOCKET_1);
            return 0;
        }

        uint8_t *ip = (uint8_t *)&addr;

        console_printf("%d.%d.%d.%d/%d\n", ip[0], ip[1], ip[2], ip[3], port);
        memset(&sin, 0, sizeof(sin)); /*Clear and fill server structure*/
        sin.msin_len = sizeof(sin);
        sin.msin_family = MN_AF_INET;
        sin.msin_port = htons(port);
        sin.msin_addr.s_addr = addr;

        if (!strcmp(SHELL_COMMAND, "connect"))  
		{
            mn_socket_set_cbs(net_test_socket, NULL, &net_test_cbs);
            rc = mn_connect(net_test_socket, (struct mn_sockaddr *)&sin);  /*Try to connect the socket for communication*/
            console_printf("mn_connect() = %d\n", rc);
        } 
		else 
		{
            mn_socket_set_cbs(net_test_socket, NULL, &net_test_cbs);
            rc = mn_bind(net_test_socket, (struct mn_sockaddr *)&sin); /*Bind socket to the settings*/
            console_printf("mn_bind() = %d\n", rc);
        }
    } 
	else if (!strcmp(SHELL_COMMAND, "listen")) 
	{
        mn_socket_set_cbs(net_test_socket, NULL, &net_listen_cbs);
        rc = mn_listen(net_test_socket, 2); /*Set socket to listen mode*/
        console_printf("mn_listen() = %d\n", rc);
    } 
	else if (!strcmp(SHELL_COMMAND, "close")) 
	{
        rc = mn_close(net_test_socket); /*Close socket*/
        console_printf("mn_close() = %d\n", rc);
        net_test_socket = NULL;

        if (net_test_socket2) 
		{
            rc = mn_close(net_test_socket2); /*Close socket 2*/
            console_printf("mn_close() = %d\n", rc);
            net_test_socket2 = NULL;
        }

    } 
	else if (!strcmp(SHELL_COMMAND, "send")) 
	{
        m = os_msys_get_pkthdr(16, 0); /*Allocate memory to m*/
        if (!m)
		{
            console_printf("out of mbufs\n");
            return 0;
        }

        rc = os_mbuf_copyinto(m, 0, SERVER_IP_ADDRESS , strlen(SERVER_IP_ADDRESS));
        if (rc < 0)
		{
            console_printf("can't copy data\n");
            os_mbuf_free_chain(m); /*Free allocated memory*/
            return 0;
        }

		if (mn_inet_pton(MN_AF_INET, PORT_NUMBER_SOCKET_1 , &addr) != 1)  /*Convert IPv4 and IPv6 addresses from text to binary form (X.X.X.X) (stored in &addr)*/
		{
			console_printf("Invalid address %s\n", SERVER_IP_ADDRESS );
			return 0;
		}
		
		port = strtoul(PORT_NUMBER_SOCKET_2, &eptr, 0); /*Converts string argument to unsigned integer to use as port*/
		if (*eptr != '\0') 
		{
			console_printf("Invalid port %s\n", PORT_NUMBER_SOCKET_2);
			return 0;
		}

		uint8_t *ip = (uint8_t *)&addr;

		console_printf("%d.%d.%d.%d/%d\n", ip[0], ip[1], ip[2], ip[3], port);
		
		memset(&sin, 0, sizeof(sin)); /*Clear and fill server structure*/
		sin.msin_len = sizeof(sin);
		sin.msin_family = MN_AF_INET;
		sin.msin_port = htons(port);
		sin.msin_addr.s_addr = addr;
		sinp = &sin;

        if (net_test_socket2) /*Decide if data should be sent to socket 1 or socket 2*/
		{
            rc = mn_sendto(net_test_socket2, m, (struct mn_sockaddr *)sinp);
        } 
		else 
		{
            rc = mn_sendto(net_test_socket, m, (struct mn_sockaddr *)sinp);
        }
		
        console_printf("mn_sendto() = %d\n", rc);

    } 
	else if (!strcmp(SHELL_COMMAND, "peer")) 
	{
        if (net_test_socket2) 
		{
            rc = mn_getpeername(net_test_socket2, (struct mn_sockaddr *)&sin);
        } 
		else 
		{
            rc = mn_getpeername(net_test_socket, (struct mn_sockaddr *)&sin);
        }

        console_printf("mn_getpeername() = %d\n", rc);

        uint8_t *ip = (uint8_t *)&sin.msin_addr;

        console_printf("%d.%d.%d.%d/%d\n", ip[0], ip[1], ip[2], ip[3], ntohs(sin.msin_port));
    } 
	else if (!strcmp(SHELL_COMMAND, "recv")) 
	{
        if (net_test_socket2) /*Decide if data should be received from socket 1 or socket 2*/
		{
            rc = mn_recvfrom(net_test_socket2, &m, (struct mn_sockaddr *)&sin);
        } 
		else 
		{
            rc = mn_recvfrom(net_test_socket, &m, (struct mn_sockaddr *)&sin);
        }

        console_printf("mn_recvfrom() = %d\n", rc);

        if (m) 
		{
            uint8_t *ip = (uint8_t *)&sin.msin_addr;
            console_printf("%d.%d.%d.%d/%d\n", ip[0], ip[1], ip[2], ip[3], ntohs(sin.msin_port));
            m->om_data[m->om_len] = '\0';
            console_printf("received %d bytes >%s<\n",
            OS_MBUF_PKTHDR(m)->omp_len, (char *)m->om_data);
            os_mbuf_free_chain(m); /*Free allocated memory*/
        }
    } 
	else if (!strcmp(SHELL_COMMAND, "mcast_join") || !strcmp(SHELL_COMMAND, "mcast_leave")) 
	{
        struct mn_mreq mm;
        int val;
		
        val = strtoul(SERVER_IP_ADDRESS , &eptr, 0); /*Converts string argument to unsigned integer to use as cast value*/

        if (*eptr != '\0') 
		{
            console_printf("Invalid itf_idx %s\n", SERVER_IP_ADDRESS );
            return 0;
        }

        memset(&mm, 0, sizeof(mm));
        mm.mm_idx = val;
        mm.mm_family = MN_AF_INET;

        if (mn_inet_pton(MN_AF_INET, PORT_NUMBER_SOCKET_1, &mm.mm_addr) != 1) /*Convert IPv4 and IPv6 addresses from text to binary form (X.X.X.X) (stored in &addr)*/
		{
            console_printf("Invalid address %s\n", SERVER_IP_ADDRESS );
            return 0;
        }

        if (!strcmp(SHELL_COMMAND, "mcast_join")) 
		{
            val = MN_MCAST_JOIN_GROUP;
        } 
		else 
		{
            val = MN_MCAST_LEAVE_GROUP;
        }

        rc = mn_setsockopt(net_test_socket, MN_SO_LEVEL, val, &mm); /*Configure socket structure*/

        console_printf("mn_setsockopt() = %d\n", rc);
    } 
	else if (!strcmp(SHELL_COMMAND, "listif")) 
	{
        struct mn_itf itf;
        struct mn_itf_addr itf_addr;
        char addr_str[48];

        memset(&itf, 0, sizeof(itf));

        while (1) 
		{
            rc = mn_itf_getnext(&itf);
            if (rc) 
			{
                break;
            }

            console_printf("%d: %x %s\n", itf.mif_idx, itf.mif_flags, itf.mif_name);

            memset(&itf_addr, 0, sizeof(itf_addr));

            while (1) 
			{
                rc = mn_itf_addr_getnext(&itf, &itf_addr);
                if (rc) 
				{
                    break;
                }

                mn_inet_ntop(itf_addr.mifa_family, &itf_addr.mifa_addr, addr_str, sizeof(addr_str)); /*Convert a binary IP address to string*/
                console_printf(" %s/%d\n", addr_str, itf_addr.mifa_plen);
            }
        }

#if MYNEWT_VAL(MCU_STM32F4) || MYNEWT_VAL(MCU_STM32F7) /*Using STM32 boards*/

    } 
	else if (!strcmp(SHELL_COMMAND, "mii")) 
	{
        extern int stm32_mii_dump(int (*func)(const char *fmt, ...));
        stm32_mii_dump(console_printf);

#endif

    } 
	else if (!strcmp(SHELL_COMMAND, "service"))
	{
        inet_def_service_init(os_eventq_dflt_get());
#if MYNEWT_VAL(BUILD_WITH_OIC)
    } 
	else if (!strcmp(SHELL_COMMAND, "oic")) 
	{
        oc_main_init((oc_handler_t *)&omgr_oc_handler);
#endif
    }
	else 
	{
        console_printf("unknown cmd\n");
    }
	
    return 0;
}


#if MYNEWT_VAL(BUILD_WITH_OIC)

static void app_get_light(oc_request_t *request, oc_interface_mask_t interface)
{
    bool value;

    if (hal_gpio_read(LED_BLINK_PIN)) 
	{
        value = true;
    } 
	else 
	{
        value = false;
    }

    oc_rep_start_root_object();

    switch (interface) 
	{
    case OC_IF_BASELINE: oc_process_baseline_interface(request->resource);
    case OC_IF_A: oc_rep_set_boolean(root, value, value);
        break;
    default:
        break;
    }
	
    oc_rep_end_root_object();
    oc_send_response(request, OC_STATUS_OK);
}



static void app_set_light(oc_request_t *request, oc_interface_mask_t interface) /*Turn on LED using an IoT like interface via send and receive Open Connectivity (OC) functions*/
{
    bool value;
    int len;
    uint16_t data_off;
    struct os_mbuf *m;
    struct cbor_attr_t attrs[] = 
	{
        [0] = 
		{
            .attribute = "value",
            .type = CborAttrBooleanType,
            .addr.boolean = &value,
            .dflt.boolean = false
        },

        [1] = 
		{

        }
    };

    len = coap_get_payload(request->packet, &m, &data_off);

    if (cbor_read_mbuf_attrs(m, data_off, len, attrs)) 
	{
        oc_send_response(request, OC_STATUS_BAD_REQUEST);
    } 
	else 
	{
        hal_gpio_write(LED_BLINK_PIN, value == true);
        oc_send_response(request, OC_STATUS_CHANGED);
    }
}


static void omgr_app_init(void) /*Another IoT interface OC application example*/
{
    oc_resource_t *res;
    oc_init_platform("MyNewt", NULL, NULL);
    oc_add_device("/oic/d", "oic.d.light", "MynewtLed", "1.0", "1.0", NULL, NULL);


    res = oc_new_resource("/light/1", 1, 0);
    oc_resource_bind_resource_type(res, "oic.r.switch.binary");
    oc_resource_bind_resource_interface(res, OC_IF_A);
    oc_resource_set_default_interface(res, OC_IF_A);

    oc_resource_set_discoverable(res);
    oc_resource_set_periodic_observable(res, 1);
    oc_resource_set_request_handler(res, OC_GET, app_get_light);
    oc_resource_set_request_handler(res, OC_PUT, app_set_light);
    oc_resource_set_request_handler(res, OC_POST, app_set_light);
    oc_add_resource(res);

    hal_gpio_init_out(LED_BLINK_PIN, 1);
}
#endif

/**
 * main
 *
 * The main task for the project. This function initializes the packages, calls
 * init_tasks to initialize additional tasks (and possibly other objects),
 * then starts serving events from default event queue.
 *
 * @return int NOTE: this function should never return!
 */

int main(void)
{
    sysinit(); /*Initialize Mynewt OS*/
    console_printf("iptest\n");
    shell_cmd_register(&net_test_cmd); /*Initialize shell*/

    while (1) 
	{
        os_eventq_run(os_eventq_dflt_get()); /*Run Mynewt OS*/
    }
}















































