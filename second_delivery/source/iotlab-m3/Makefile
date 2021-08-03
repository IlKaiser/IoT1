# name of your application
APPLICATION = iot_lab_smart_home

# If no BOARD is found in the environment, use this default:
BOARD ?= iotlab-m3
DEFAULT_CHANNEL ?= 22

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../../../RIOT

# Include packages that pull up and auto-init the link layer.
# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif
# Activate ICMPv6 error messages
USEMODULE += gnrc_icmpv6_error
# Specify the mandatory networking modules for IPv6 and UDP
USEMODULE += gnrc_ipv6_router_default
USEMODULE += gnrc_udp
# Add a routing protocol
USEMODULE += gnrc_rpl
USEMODULE += auto_init_gnrc_rpl
# This application dumps received packets to STDIO using the pktdump module
USEMODULE += gnrc_pktdump
# Additional networking modules that can be dropped if not needed
USEMODULE += gnrc_icmpv6_echo
# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += netstats_l2
USEMODULE += netstats_ipv6
USEMODULE += netstats_rpl

USEMODULE += gnrc_sock_check_reuse
USEMODULE += gnrc_sock_udp
USEMODULE += gnrc_ipv6


# Print effin floats
USEMODULE += printf_float

# Random
USEMODULE += random


# Include MQTT-S client
USEMODULE += emcute

# Optionally include DNS support. This includes resolution of names at an
# upstream DNS server and the handling of RDNSS options in Router Advertisements
# to auto-configure that upstream DNS server.
# USEMODULE += sock_dns              # include DNS client
# USEMODULE += gnrc_ipv6_nib_dns     # include RDNSS option handling

# Uncomment this to enable legacy support of netdev for IEEE 802.15.4 radios.
# USEMODULE += netdev_ieee802154_legacy

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Instead of simulating an Ethernet connection, we can also simulate
# an IEEE 802.15.4 radio using ZEP
USE_ZEP ?= 0

# set the ZEP port for native
ZEP_PORT_BASE ?= 17754
ifeq (1,$(USE_ZEP))
  TERMFLAGS += -z [::1]:$(ZEP_PORT_BASE)
  USEMODULE += socket_zep
endif


#
# Ethos
#

# Ethos/native TAP interface and UHCP prefix can be configured from make command
TAP ?= tap0
IPV6_PREFIX ?= fe80:2::/64

#
# MQTT-SN
#
# The Broker address, port and the default MQTT topic to subscribe.
SERVER_ADDR = fe80::1
SERVER_PORT = 1885
MQTT_TOPIC = tft

CFLAGS += -DSERVER_ADDR='"$(SERVER_ADDR)"'
CFLAGS += -DSERVER_PORT=$(SERVER_PORT)
CFLAGS += -DMQTT_TOPIC='"$(MQTT_TOPIC)"'

ETHOS_BAUDRATE ?= 115200
#include $(CURDIR)/Makefile.ethos.conf




# Uncomment the following 2 lines to specify static link lokal IPv6 address
# this might be useful for testing, in cases where you cannot or do not want to
# run a shell with ifconfig to get the real link lokal address.
#IPV6_STATIC_LLADDR ?= '"fe80::cafe:cafe:cafe:1"'
#CFLAGS += -DGNRC_IPV6_STATIC_LLADDR=$(IPV6_STATIC_LLADDR)

# Uncomment this to join RPL DODAGs even if DIOs do not contain
# DODAG Configuration Options (see the doc for more info)
# CFLAGS += -DCONFIG_GNRC_RPL_DODAG_CONF_OPTIONAL_ON_JOIN

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include

ifneq (,$(filter gnrc_sock,$(USEMODULE))) 
   USEMODULE_INCLUDES += $(RIOTBASE)/sys/net/gnrc/sock/include
endif

# Set a custom channel if needed
include $(RIOTMAKE)/default-radio-settings.inc.mk
