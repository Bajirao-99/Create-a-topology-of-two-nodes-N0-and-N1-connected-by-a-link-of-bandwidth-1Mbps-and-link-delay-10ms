An ns3 simulation script to compare various TCP variants under congestion

Topology:
 * Create a topology of two nodes N0 and N1 connected by a link of bandwidth 1 Mbps and link delay 10 ms. Use a
drop-tail queue at the link.
 * n0 ----------- n1

Simulation:
There is a TCP agent at N0 which creates FTP traffic destined for N1. There are 5 CBR traffic agents of rate 300 Kbps each at N0 destined for N1.
The timing of the flows is as follows:
 * FTP starts at 0 sec and continues till the end of simulation
 * CBR1 starts at 200 ms and continues till end
 * CBR2 starts at 400 ms and continues till end
 * CBR3 starts at 600 ms and stops at 1200 ms
 * CBR4 starts at 800 ms and stops at 1400 ms
 * CBR5 starts at 1000 ms and stops at 1600 ms
 * Simulation runs for 1800 ms

Installation:
Steps:
 1. Install ns3 and put ns3.cc in the scratch directory.
 2. Run the script using "./ns3 run scratch/Assignment3.cc", this will run for all types of tcp protocols and genetaes respected files.
 3. Enter thw gnuplot command 

Aftering entering gnuplot command in terminal, ebter following commands:

1. filenames = "TcpNewReno TcpHybla TcpWestWood TcpScalable TcpVegas"
2. set xlabel "Time(s)"
3. set ylabel "Cumulative Received Bytes"
4. plot for [file in filenames] file."_bytes_received.dat" using 1:2 title file with lines

5. set ylabel "Congestion Window Size (Bytes)"
6. plot for [file in filenames] file."_congestion_window_size.dat" using 1:2 title file  with lines

7. set ket top left
8. set ylabel "No of Packets Dropped"
9. plot for [file in filenames] file."_dropped_packets.dat" using 1:2 title file with linespoints

Plots: 
 * "Congestion Window size vs Time" plot is stored in the file named "Congestion_window_size.png" file
 * "Total Bytes Received vs Time" plot is stored in the file named "Bytes_Received.png" file
 * "No of Dropped packets vs Time" plot is stored in the file named "Dropped_Packets.png" file
