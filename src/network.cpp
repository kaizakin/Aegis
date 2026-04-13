#include "network.hpp"
#include <cstdlib>

// system runs a command just like how it is run in terminal
// it creates a bash process runs the command and waits for the result
static void cmd(const std::string& c){
  system(c.c_str());
}

// constructor
Network::Network(pid_t pid) : pid(pid) {
  veth_host = "veth0_" + std::to_string(pid);

  // Only the host-side name needs to be unique per container.
  // container side gets its own network so there is no need for unique name.
  veth_container = "veth1";
  bridge = "bridge_" + std::to_string(pid); // real production runtime would reuse bridge but lazy ass me just creating a bridge for every container
}

// create a virutal ethernet to establish internet connection to containers
void Network::setup(){
  cmd("ip link add " + veth_host + " type veth peer name " + veth_container); // create a cable
  cmd("ip link set " + veth_container + " netns " + std::to_string(pid)); // push one end to the container

  cmd("brctl addbr " + bridge); // create a switch
  cmd("brctl addif " + bridge + " " + veth_host); // plug host end of the cable in the switch

  //power on both the interfaces
  cmd("ip link set " + veth_host + " up");
  cmd("ip link set " + bridge + " up");

  cmd("ip addr add 10.0.0.1/24 dev " + bridge); // assign an ip address to the bridge
  // bridge is gonna act as a gateway that route requests back and forth of the container and the host 
  // so give it a private ip
  
  // Allow traffic from our bridge to the main network interface
  cmd("iptables -A FORWARD -i " + bridge + " -j ACCEPT");

  // Allow established traffic to come back from the main interface to our bridge
  cmd("iptables -A FORWARD -o " + bridge + " -m state --state RELATED,ESTABLISHED -j ACCEPT");
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
  cmd("brctl delbr " + bridge);
}
