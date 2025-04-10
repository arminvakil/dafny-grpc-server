# Setup Guide for Load Balancing Dafny GRPC Server

We used cloudlab nodes and this guide will be based on that assumption. To access the repository from where it is set up on the node, replace the node IDs and
username with your own in setup-nodes.sh and setup-ssh-keys.sh.

## Setup of remote nodes
`./setup-ssh-keys.sh` and
`./setup-nodes.sh`.

## Edit your config file to look like this:
Host apt162.apt.emulab.net
  HostName apt162.apt.emulab.net
  User {your username}
  IdentityFile {path to your rsa file}

Then do ctrl+shift+p and remote-ssh into the first node you list. FInd the IP for the first node (you only need to do this for the manager node).

## Make the first node the manager node:
sudo docker swarm init --advertise-addr {MANAGER IP}

This will get an instruction like 'docker swarm join --token ajsdlfkjadslkfjdskjfdskjfldks'; save this command. In your local window, ssh into the other nodes in the same way.

## Make the other nodes worker nodes:
Use the token given to you from the manager node and paste it into the terminal of the worker nodes.
sudo docker swarm join --token SWMTKN-1-3wuyivotesgi4fhom3fynbcsloystqobjcfuug8ol1doxqf2lz-916f3s7xs8cm961ty1khzz5bs {Manager IP}:Port

## Run the dafny grpc server:
In the manager node, navigate to proj/dafny-grpc-server. Run these commands.

### Deploy the docker-compose.yml
`sudo docker stack deploy -c docker-compose.yml grpc_stack`
You can use these commands to see the status of the containers:
`sudo docker service ls`
`sudo docker service ps grpc_stack_grpc`

### Build the Dockerfile
`sudo docker build -t sudeeptirao/dafny_grpc -f Dafny-docker/Dockerfile .`

### Send verification requests to the server
tmp.dfy holds a proxy request, but any dafny program can be submitted as an argument for verification:
`python3 src/dafny-client.py 127.0.0.1:50052 tmp.dfy`

```sh
# This is irrelevant, but this command is for the default client.js file and default Dockerfile we originally had, for insight:
sudo docker run --network host -i -t sudeeptirao/grpc_lb:latest ./client.js --target localhost:50052 --iterations 10000 --batchSize 100
```