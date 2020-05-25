#!/bin/bash

#pass parameter 
# while getopts u:d:p:f: option

# while getopts f:p:s: option
# do
# case "${option}"
# in
# f) FLOW_COUNT=${OPTARG};;
# p) GPFC_PAUSE_PRIORITY=${OPTARG};;
# s) FLOW_SIZE=${OPTARG};;
# # f) FORMAT=${OPTARG};;
# esac
# done

# runsim function variables: flow number, flow size in byte, max_priority, pause_rank, 
function run_sim { 
FLOW_COUNT=$1
FLOW_SIZE=$2
FLOW_PRIORITY_MAX=$3
GPFC_PAUSE_PRIORITY=$4

TEST_CASE_NAME="0520-GPFC-TEST-STATIC-flowsize-${FLOW_SIZE}Byte-${FLOW_COUNT}flow-16pri-${GPFC_PAUSE_PRIORITY}pause"

#Set Variables

PFC_TH_MAX=700
PFC_TH_MIN=128
GPFC_TH_MAX=400
GPFC_TH_MIN=128

FLOW_PRIORITY_MIN=1
FLOW_PRIORITY_MAX=16
# GPFC_PAUSE_PRIORITY=9


QUEUE_SIZE="1000p"

#FLOW_COUNT=64 # flow per each node.
INPUT_BAND=4000 # 4096Mbps -> 4Gbps. 400-> 400Mbps
FLOW_PKT_SIZE=512 # packet length is 512 byte

# FLOW_SIZE=100000 # unit Byte.
LINK_SPEED="1000Mbps"
LINK_DELAY="1us"


PRIORITY_FILE_NAME="priority.txt"

isGPFC="false"
DEQUEUE_MODE="PQ"
ENQUEUE_MODE="PI"
PAUSE_MODE="STATIC"
LOG_FILE_NAME=log-${TEST_CASE_NAME}-1.log

echo "Start PFC Simulation"
# echo "Start PQ Simulation"
# Run PFC.

./waf --run "scratch/jk_test1_simple_pfc 
--input_band=${INPUT_BAND}
--pkt_size=${FLOW_PKT_SIZE}
--flow_count=${FLOW_COUNT}
--flow_size=${FLOW_SIZE}
--link_speed=${LINK_SPEED}
--link_delay=${LINK_DELAY}
--priority_min=${FLOW_PRIORITY_MIN}
--priority_max=${FLOW_PRIORITY_MAX}
--is_gpfc=${isGPFC}
--queue_max_size_in_pkt=${QUEUE_SIZE}
--pfc_threshold_max=${PFC_TH_MAX}
--pfc_threshold_min=${PFC_TH_MIN}
--gpfc_threshold_max=${GPFC_TH_MAX}
--gpfc_threshold_min=${GPFC_TH_MIN}
--gpfc_pause_priority=${GPFC_PAUSE_PRIORITY}
--flow_monitor_xml_file_name=${TEST_CASE_NAME}-1.flowmon
--log_file_name=${TEST_CASE_NAME}-flow-state.txt
--dequeue_mode=${DEQUEUE_MODE}
--enqueue_mode=${ENQUEUE_MODE}
--pause_mode=${PAUSE_MODE}
" 2>${LOG_FILE_NAME}

# Run GPFC in static 

isGPFC="true"
PAUSE_MODE="STATIC"
# DEQUEUE_MODE="RR"
LOG_FILE_NAME=log-${TEST_CASE_NAME}-2.log

echo "Start GPFC Static Simulation"
# echo "Start RR Simulation"


./waf --run "scratch/jk_test1_simple_pfc 
--input_band=${INPUT_BAND}
--pkt_size=${FLOW_PKT_SIZE}
--flow_count=${FLOW_COUNT}
--flow_size=${FLOW_SIZE}
--link_speed=${LINK_SPEED}
--link_delay=${LINK_DELAY}
--priority_min=${FLOW_PRIORITY_MIN}
--priority_max=${FLOW_PRIORITY_MAX}
--is_gpfc=${isGPFC}
--queue_max_size_in_pkt=${QUEUE_SIZE}
--pfc_threshold_max=${PFC_TH_MAX}
--pfc_threshold_min=${PFC_TH_MIN}
--gpfc_threshold_max=${GPFC_TH_MAX}
--gpfc_threshold_min=${GPFC_TH_MIN}
--gpfc_pause_priority=${GPFC_PAUSE_PRIORITY}
--flow_monitor_xml_file_name=${TEST_CASE_NAME}-2.flowmon
--log_file_name=${TEST_CASE_NAME}-flow-gfc-state.txt
--dequeue_mode=${DEQUEUE_MODE}
--enqueue_mode=${ENQUEUE_MODE}
--pause_mode=${PAUSE_MODE}
" 2>${LOG_FILE_NAME}

# Run GPFC in dynamic

# isGPFC="true"
# PAUSE_MODE="DYNAMIC"
# DEQUEUE_MODE="PQ"


# echo "Start GPFC Dynamic Simulation"
# echo "Start RR Simulation"


# ./waf --run "scratch/jk_test1_simple_pfc 
# --input_band=${INPUT_BAND}
# --pkt_size=${FLOW_PKT_SIZE}
# --flow_count=${FLOW_COUNT}
# --flow_size=${FLOW_SIZE}
# --link_speed=${LINK_SPEED}
# --link_delay=${LINK_DELAY}
# --priority_min=${FLOW_PRIORITY_MIN}
# --priority_max=${FLOW_PRIORITY_MAX}
# --is_gpfc=${isGPFC}
# --queue_max_size_in_pkt=${QUEUE_SIZE}
# --pfc_threshold_max=${PFC_TH_MAX}
# --pfc_threshold_min=${PFC_TH_MIN}
# --gpfc_threshold_max=${GPFC_TH_MAX}
# --gpfc_threshold_min=${GPFC_TH_MIN}
# --gpfc_pause_priority=${GPFC_PAUSE_PRIORITY}
# --flow_monitor_xml_file_name=${TEST_CASE_NAME}-3.flowmon
# --log_file_name=${TEST_CASE_NAME}-flow-gfc-state.txt
# --dequeue_mode=${DEQUEUE_MODE}
# --pause_mode=${PAUSE_MODE}
# " 2>log-${TEST_CASE_NAME}-3.log


echo "Analyze Result"

# call python to draw chart.
# x for GPFC
# y for PFC
# p for priority list.
# python xml_parsing.py -x ${TEST_CASE_NAME}-1.flowmon -y ${TEST_CASE_NAME}-2.flowmon -z ${TEST_CASE_NAME}-3.flowmon -p ${PRIORITY_FILE_NAME}
python xml_parsing_2.py -x ${TEST_CASE_NAME}-1.flowmon -y ${TEST_CASE_NAME}-2.flowmon -p ${PRIORITY_FILE_NAME}

echo "save log files"
mkdir -p ${TEST_CASE_NAME}
mv ${PRIORITY_FILE_NAME} ${TEST_CASE_NAME}/
mv ${TEST_CASE_NAME}-1.flowmon ${TEST_CASE_NAME}/
mv log-${TEST_CASE_NAME}-1.log ${TEST_CASE_NAME}/

mv ${TEST_CASE_NAME}-2.flowmon ${TEST_CASE_NAME}/
mv log-${TEST_CASE_NAME}-2.log ${TEST_CASE_NAME}/

# copy python result
mv result.out ${TEST_CASE_NAME}/

}



# for i in {1..16}
# do
# 	echo "===================================="
# 	echo "Start Simulation with Pause Rank  $i"
# 	echo "===================================="

# 	# runsim function variables: flow number, flow size in byte, max_priority, pause_rank, 
# 	run_sim 64 10000 16 $i

# done

# run_sim 64 10000 16 3
 run_sim 64 1000000 16 4

# for flow_size in 10000 100000 1000000
# do

# 	for flow_count in 64 128
# 	do
# 		for pause_rank in {1..16}
# 		do
# 			# flow_size=10000
# 			max_priority=16

# 			# run_sim $flow_count 10000 16 3


# 			echo "===================================="
# 			echo "Start Simulation with "
# 			echo "Max_Priority  : $max_priority"
# 			echo "Flow Size     : $flow_size"
# 			echo "Flow Count    : $flow_count"
# 			echo "Pause_Priority: $pause_rank"
# 			echo "===================================="
			
# 			run_sim $flow_count $flow_size $max_priority $pause_rank
# 		done


# 	done
# done

