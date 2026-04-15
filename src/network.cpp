#include "network.hpp"
#include "utils.hpp"

using utils::cmd;

// constructor
Network::Network(pid_t pid) : pid(pid) {
  veth_host = "veth0_" + std::to_string(pid);

  // Only the host-side name needs to be unique per container.
  // container side gets its own network so there is no need for unique name.
  veth_container = "veth1";
  bridge = "bridge_" + std::to_string(pid); // real production runtime would reuse bridge but lazy ass me just creating a bridge for every container
}

// RAII (Resource Acquisition Is Initialization)
// lifecycle of resources is tied to the lifecycle of the object

Network::~Network(){
  teardown(); 
}

// create a virutal ethernet to establish internet connection to containers
void Network::setup(){
  cmd("ip link add " + veth_host + " type veth peer name " + veth_container); // create a cable
  cmd("ip link set " + veth_container + " netns " + std::to_string(pid)); // push one end to the container

  cmd("ip link add name " + bridge + " type bridge"); // create a bridge
  cmd("ip link set " + veth_host + " master " + bridge); // attach host veth to the bridge

  //power on both the interfaces
  cmd("ip link set " + veth_host + " up");
  cmd("ip link set " + bridge + " up");

  // enable IP forwarding
  // so host acts like a router between container and internet
  cmd("sudo sysctl -w net.ipv4.ip_forward=1");

  cmd("ip addr add 10.0.0.1/24 dev " + bridge); // assign an ip address to the bridge
  // bridge is gonna act as a gateway that route requests back and forth of the container and the host 
  // so give it a private ip
  
  // Allow traffic from our bridge to the main network interface
  cmd("iptables -A FORWARD -i " + bridge + " -j ACCEPT");

  // Allow established traffic to come back from the main interface to our bridge
  cmd("iptables -A FORWARD -o " + bridge + " -m state --state RELATED,ESTABLISHED -j ACCEPT");

  // for every network request coming from container rewrite container ip with host ip
  // so the response actually reaches back the host
  // internet doesn't know how to send response to private ip
  cmd("sudo iptables -t nat -A POSTROUTING -s 10.0.0.0/24 -o wlan0 -j MASQUERADE");
}

void Network::teardown() {
  // Delete iptables rules
  cmd("iptables -D FORWARD -i " + bridge + " -j ACCEPT");
  cmd("iptables -D FORWARD -o " + bridge + " -m state --state RELATED,ESTABLISHED -j ACCEPT");

  // delete veth pair this removes both ends
  cmd("ip link delete " + veth_host);

  // bring the bridge down
  cmd("ip link set " + bridge + " down");

  // delete bridge
  cmd("ip link delete " + bridge + " type bridge");
}
