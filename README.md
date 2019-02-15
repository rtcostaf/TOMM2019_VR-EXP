# VR-EXP - An Experimentation Platform for Adaptive Virtual Reality Video Streaming

Getting started with VR-EXP

The easiest way to get started on VR-EXP is to download the pre-configured virtual machine.

1. Download and install the Virtual Box virtualisation system.
https://www.virtualbox.org/

2. Download the Virtual Machine (approx. 6GB).
https://drive.google.com/open?id=1QgUPzpEth5eqsfsbdX0vxV8r2EuBHt_o

3. After the download is complete, import the vrexp.ova file into your system by using the option VirtualBox -> “Import Appliance”.

4. (Optional) If you want to access the virtual machine via SSH, it will be necessary to add a Host Only interface (VirtualBox preferences). Next, run the following command: ssh vrexp@192.168.56.101
Login credentials:
Username: vrexp
Password: vrexp

5. Using VR-EXP:

Quick test:
Usage: vrexp <destination> <video_id> <segment_count> <uuid> <tracefile> <timeout> <tilling scheme - vertical split (lines) - horizontal split (columns)> <error rate> <number of threads> <ABR heuristic> <playout buffer_size>

Example (as root):
# ssh vrexp@192.168.56.101
# sudo su -
# cd /home/vrexp/
# vrexp localhost v1 60 id0001 /usr/local/src/viewport_traces/per_user/user_1/video_1.txt 90 4 8 0 1 1 2

Output file: /home/vrexp/videologs/id0001.log
Tile-based VR videos directory: /var/www/html/
Head track traces: /usr/local/src/viewport_traces/

(Optional) Automated execution with enforced network performance: 

a. Define the experiment parameters in the following files:
- run.sh
- Topo.py
- Topology.txt

If you want to perform a simple test, the provided example files should be ok.

b. (Optional) - To enforce controlled network performance conditions, run the SDN controller:
cd /usr/local/src
nohup sudo ryu-manager Controller.py &

c. (Optional) - Simulate a network topology.
# /usr/local/src/netconf7/Gent_topo_2.py&

d. Run apache2 on the mininet CDN node:
# /home/mininet/mininet/util/m cdn1 bash /home/rtcosta/start_apache.sh cdn1

e. (Optional) - For automated execution, use the sample script /usr/local/bin/run.sh (the script run.sh is responsible for both varying the network conditions and executing the VR-EXP)
# nohup run.sh &

f. For a standalone execution using mininet:
# /home/mininet/mininet/util/m u001 /usr/local/bin/vrexp 10.0.0.251 v1 60 id0001 /usr/local/src/viewport_traces/per_user/user_1/video_1.txt 90 4 12 30 6 1 2 

Extra:
If you need to recompile VR-EXP, you will need the following libs:

pthread.h
stdio.h
stdlib.h
unistd.h
string.h
curl.h
errno.h
stdio.h
math.h

To compile the source code:
# gcc vrexp.c -lcurl -lpthread -o vrexp
