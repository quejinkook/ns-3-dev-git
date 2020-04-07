#! /usr/bin/env python3

import xml.dom.minidom
from decimal import Decimal
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import optparse
import sys

def cdf_fct(flow_mon_list):

	sortedlist = sorted(flow_mon_list, key=lambda k: k['flow_completion_time']) 

	max_time = sortedlist[-1]['flow_completion_time']

	max_time = int(max_time) + 10

	cdf_list_y = []
	cdf_list_x = []
	cdf_list_percentage = [0,0,0,0,0,0,0,0,0,0]

	cdf_50 = 0
	cdf_90 = 0

	for i in range(1,max_time):

		finish_count = 0

		for item in sortedlist:
			item_fct = int(item['flow_completion_time'])
			if item_fct <= i:
				finish_count = finish_count + 1

		# calculate cdf y value.
		cdf_y = float(finish_count) / float(len(sortedlist)) * 100.0

		for j in range(0,10):
			if(cdf_list_percentage[j] == 0 and cdf_y >= float((j+1)*10)):
				cdf_list_percentage[j] = i


		if(cdf_50 == 0 and cdf_y >= 50.0):
			cdf_50 = i

		if(cdf_90 == 0 and cdf_y >= 90.0):
			cdf_90 = i

		cdf_list_y.append(cdf_y)
		cdf_list_x.append(i)

	return cdf_list_x, cdf_list_y, cdf_50, cdf_90, cdf_list_percentage


def autolabel(gpfc_rect,pfc_rect, ax):
    """Attach a text label above each bar in *rects*, displaying its height."""

    for i in range (0,len(gpfc_rect)):

    # for rect1 in rects1:
        height1 = gpfc_rect[i].get_height()
        height2 = pfc_rect[i].get_height()

        # percentage = float(abs(height2-height1)) * 100.0 / float(height2)
        percentage = float(height2-height1) * 100.0 / float(height2)

        font_color = "black"
        if (height2 < height1):
        	font_color ="red"
        ax.annotate('{0:.2f}'.format(percentage),
                    xy=(gpfc_rect[i].get_x() + gpfc_rect[i].get_width() / 2, max(height1,height2)),
                    xytext=(0, 3),  # 3 points vertical offset
                    textcoords="offset points",
                    ha='center', va='bottom', color = font_color)


def print_avg_fct_from_sorted_list(list):

	temp_priority = int(list[0]['priority'])
	temp_count = 0
	temp_sum = 0

	avg_per_priority = []

	print "=========   Average   ============="

	for item in list:
		priority = int(item['priority'])
		fct = float(item['flow_completion_time'])

		if(temp_priority != priority):
			#calculate average and 
			#reset temp parameters 
			#ready to move to next,

			if(temp_count == 0):
				avg = 0
			else:
				avg = temp_sum / temp_count
			
			print "%d's average FCT is %f" %(temp_priority,avg)
			avg_per_priority.append(avg)

			temp_sum = fct
			temp_count = 1
			temp_priority = priority

		else:
			#cumulate the values
			temp_count = temp_count + 1
			temp_sum = temp_sum + fct

	#calculate average for the last one

	if(temp_count == 0):
		avg = 0
	else:
		avg = temp_sum / temp_count

	print "%d's average FCT is %f" %(temp_priority,avg)
	avg_per_priority.append(avg)

	return avg_per_priority

def print_avg_fct_diff(list1, list2):
	result_list = []
	if(len(list1) != len(list2)):
		print "[Error] different size: at print_avg_fct_diff function"

	print "==========Diff (a-b)/a============"

	for i in range(0, len(list1)):
		diff_per = 0

		if(list1[i] != 0):
			diff_per = (list1[i] - list2[i]) / list1[i]
		
		print (i+1),"th diff", diff_per
		result_list.append(diff_per)

	return result_list



def main():

	parser = optparse.OptionParser()

	parser.add_option('-x', '--flow_xml1',
	    action="store", dest="f1",
	    help="flow monitor xml file1 name", default="")

	parser.add_option('-y', '--flow_xml2',
		    action="store", dest="f2",
		    help="flow monitor xml file2 name", default="")

	# parser.add_option('-z', '--flow_xml3',
	# 	    action="store", dest="f3",
	# 	    help="flow monitor xml file3 name", default="")

	parser.add_option('-p', '--priority_file',
		    action="store", dest="p",
		    help="file contains priority list", default="")
	options, args = parser.parse_args()


	# read flow priority list

	f = open(options.p, "r")
	priority_str = f.readline()
	PriorityList = priority_str.split(',')

	print PriorityList
	print "length is " + str(len(PriorityList))

	doc = xml.dom.minidom.parse(options.f1)
	doc2 = xml.dom.minidom.parse(options.f2)
	# doc3 = xml.dom.minidom.parse(options.f3)
	
	# doc2 = xml.dom.minidom.parse("PFC.flowmon")
	# print doc.nodeName
	# print doc.firstChild.tagName

	# get flow stats

	flowStats = doc.getElementsByTagName("FlowStats")
	flows = flowStats[0].getElementsByTagName("Flow")

	flowStats2 = doc2.getElementsByTagName("FlowStats")
	flows2 = flowStats2[0].getElementsByTagName("Flow")

	# flowStats3 = doc3.getElementsByTagName("FlowStats")
	# flows3 = flowStats3[0].getElementsByTagName("Flow")

	print ( "Flow Stats, Total: %d " %flows.length)

	# PriorityList = [8,6,7,1,1,4,7,8,1,3,5,4,1,6,2,6,8,2,8,5,7,3,7,1,6,8,5,6,8,2,2,2,4,3,1,8,2,4,8,7,4,6,2,4,1,3,7,6,1,5,4,4,1,1,4,3,3,1,7,3,1,2,1,7]
	FlowList = []
	FlowList2 = []
	FlowList3 = []


	for i in range(0,flows.length):
		flow = flows[i]
		
		flowDict = {}
		flowDict["priority"] = PriorityList[i]
		flowDict["id"] = flow.getAttribute("flowId")
		flowDict["txPackets"] = flow.getAttribute("txPackets")
		flowDict["rxPackets"] = flow.getAttribute("rxPackets")
		flowDict["lostPackets"] = flow.getAttribute("lostPackets")
		flowDict["start_time"] = flow.getAttribute("timeFirstTxPacket")
		flowDict["finish_time"] = flow.getAttribute("timeLastRxPacket")
		flowDict["delay_sum"] = flow.getAttribute("delaySum")
		start_time_str = flowDict["start_time"][1:-2]
		finish_time_str = flowDict["finish_time"][1:-2]

		flow_completion_time = (Decimal(finish_time_str) - Decimal(start_time_str)) / 1000000
		flowDict["flow_completion_time"] = flow_completion_time
		
		FlowList.append(flowDict)

	for i in range(0,flows2.length):
		flow = flows2[i]
		
		flowDict = {}
		flowDict["priority"] = PriorityList[i]
		flowDict["id"] = flow.getAttribute("flowId")
		flowDict["txPackets"] = flow.getAttribute("txPackets")
		flowDict["rxPackets"] = flow.getAttribute("rxPackets")
		flowDict["lostPackets"] = flow.getAttribute("lostPackets")
		flowDict["start_time"] = flow.getAttribute("timeFirstTxPacket")
		flowDict["finish_time"] = flow.getAttribute("timeLastRxPacket")
		flowDict["delay_sum"] = flow.getAttribute("delaySum")
		start_time_str = flowDict["start_time"][1:-2]
		finish_time_str = flowDict["finish_time"][1:-2]

		#flow completion time in milliseconds.

		flow_completion_time = (Decimal(finish_time_str) - Decimal(start_time_str)) / 1000000  
		flowDict["flow_completion_time"] = flow_completion_time
		
		FlowList2.append(flowDict)

	# for i in range(0,flows3.length):
	# 	flow = flows3[i]
		
	# 	flowDict = {}
	# 	flowDict["priority"] = PriorityList[i]
	# 	flowDict["id"] = flow.getAttribute("flowId")
	# 	flowDict["txPackets"] = flow.getAttribute("txPackets")
	# 	flowDict["rxPackets"] = flow.getAttribute("rxPackets")
	# 	flowDict["lostPackets"] = flow.getAttribute("lostPackets")
	# 	flowDict["start_time"] = flow.getAttribute("timeFirstTxPacket")
	# 	flowDict["finish_time"] = flow.getAttribute("timeLastRxPacket")
	# 	flowDict["delay_sum"] = flow.getAttribute("delaySum")
	# 	start_time_str = flowDict["start_time"][1:-2]
	# 	finish_time_str = flowDict["finish_time"][1:-2]

	# 	#flow completion time in milliseconds.

	# 	flow_completion_time = (Decimal(finish_time_str) - Decimal(start_time_str)) / 1000000  
	# 	flowDict["flow_completion_time"] = flow_completion_time
		
	# 	FlowList3.append(flowDict)


	# print FlowList


	# Sort FlowList with flow priority.
	sortedlist = sorted(FlowList, key=lambda k: int(k['priority'])) 
	sortedlist2 = sorted(FlowList2, key=lambda k: int(k['priority'])) 
	# sortedlist3 = sorted(FlowList3, key=lambda k: k['priority']) 


	names = []
	values1 = []
	values2 = []
	# values3 = []

	for i in range (0,len(sortedlist)):
		if ((sortedlist[i]['id'] != sortedlist2[i]['id']) and (sortedlist[i]['id'] != sortedlist3[i]['id']) ):
			print ("[sequence check error]Two Sorted List Sequence is not same at id %d" %i)
			return 0
		else:
			print ("[sequence check pass] Two Sorted List Sequence is same at id %d" %i)


	for i in range (0,len(sortedlist)):
		if (int(sortedlist[i]['lostPackets']) > 0):
			print ("[Loss check Error]Loss Found At %s, flow id:%d, txPacket:%d, rxPacket:%d, lossPacket:%d" 
				%(options.f1, int(sortedlist[i]['id']), int(sortedlist[i]['txPackets']), int(sortedlist[i]['rxPackets']), int(sortedlist[i]['lostPackets'])))
	
	for i in range (0,len(sortedlist)):
		if (int(sortedlist2[i]['lostPackets']) > 0):
			print ("[Loss check Error]Loss Found At %s, flow id:%d, txPacket:%d, rxPacket:%d, lossPacket:%d" 
				%(options.f2, int(sortedlist2[i]['id']), int(sortedlist2[i]['txPackets']), int(sortedlist2[i]['rxPackets']), int(sortedlist2[i]['lostPackets'])))

	# for i in range (0,len(sortedlist)):
	# 	if (int(sortedlist3[i]['lostPackets']) > 0):
	# 		print ("[Loss check Error]Loss Found At %s, flow id:%d, txPacket:%d, rxPacket:%d, lossPacket:%d" 
	# 			%(options.f3, int(sortedlist3[i]['id']), int(sortedlist3[i]['txPackets']), int(sortedlist3[i]['rxPackets']), int(sortedlist3[i]['lostPackets'])))



	for item in sortedlist:
		# print item
		print ("flow Prority %s, FLow ID %s, lostPackets %s, flow_completion_time %s" 
			%(item["priority"],item['id'],item['lostPackets'],item['flow_completion_time']))
		name_str = item['id'] + "\n(" + item['priority']+")"
		# name_str = "Flow" + item['id'] + "(" + flowDict["txPackets"]+")"
		names.append(name_str)
		values1.append(item["flow_completion_time"])

	for item in sortedlist2:
		# print item
		print ("flow Prority %s, FLow ID %s, lostPackets %s, flow_completion_time %s" 
			%(item["priority"],item['id'],item['lostPackets'],item['flow_completion_time']))
		name_str = "Flow" + item['id'] + "(" + flowDict["txPackets"]+")"
		# names.append(name_str)
		values2.append(item["flow_completion_time"])

	# for item in sortedlist3:
	# 	# print item
	# 	print ("flow Prority %s, FLow ID %s, lostPackets %s, flow_completion_time %s" 
	# 		%(item["priority"],item['id'],item['lostPackets'],item['flow_completion_time']))
	# 	name_str = "Flow" + item['id'] + "(" + flowDict["txPackets"]+")"
	# 	# names.append(name_str)
	# 	values3.append(item["flow_completion_time"])


	# print flowDict["flow_completion_time"]
	# print values2
	# call cdf



	# Analysis.

	# calcualte average per priority

	print ("============================================")
	print ("               1. Average")
	print ("============================================")

	avg_list_1 = print_avg_fct_from_sorted_list(sortedlist)
	avg_list_2 = print_avg_fct_from_sorted_list(sortedlist2)

	diff_list = print_avg_fct_diff(avg_list_1, avg_list_2)	



	print "avg list 1 ", avg_list_1
	print "avg list 2 ", avg_list_2

	print "diff_list", diff_list

	print ("============================================")
	print ("               2. CDF")
	print ("============================================")
	# Calculate CDF
	cdf_x_1, cdf_y_1, cdf_50_1, cdf_90_1, cdf_list_1 = cdf_fct(FlowList)
	cdf_x_2, cdf_y_2, cdf_50_2, cdf_90_2, cdf_list_2 = cdf_fct(FlowList2)
	# cdf_x_3, cdf_y_3 = cdf_fct(FlowList3)


	cdf_50_improve = float(cdf_50_1 - cdf_50_2) /  float(cdf_50_1)
	cdf_90_improve = float(cdf_90_1 - cdf_90_2) /  float(cdf_90_2)

	print ("============================================")
	# print ("Trail1: Time for CDF 50 is %f, CDF 90 is %f" %(cdf_50_1,cdf_90_1))
	print "Trail1: Time for CDF list", cdf_list_1
	# print ("Trail2: Time for CDF 50 is %f, CDF 90 is %f" %(cdf_50_2,cdf_90_2))
	print "Trail2: Time for CDF list", cdf_list_2
	# print ("CDF Improve: CDF 50 is %f, CDF 90 is %f" %(cdf_50_improve, cdf_90_improve))

	# CDF n percentage improvement
	for i in range(0,len(cdf_list_1)):
		cdf_percentage = float(cdf_list_1[i] - cdf_list_2[i]) / float(cdf_list_1[i])
		print("CDF %dth : Trail 1 - %.2f, Trail2 - %.2f, diff - %.2f" %((i+1)*10, cdf_list_1[i], cdf_list_2[i], cdf_percentage))







	x = np.arange(len(names))  # the label locations
	width = 0.2  # the width of the bars

	fig, axs = plt.subplots(2,1)

	#==============================
	#subplot 1 FCT Completion Time.
	#==============================

	ax = axs[0]

	rects1 = ax.bar(x - width, values1, width, label=options.f1)
	rects2 = ax.bar(x, values2, width, label=options.f2)
	# rects3 = ax.bar(x + width, values3, width, label=options.f3)

	# Add some text for labels, title and custom x-axis tick labels, etc.
	ax.set_ylabel('Time (unit millisecond)')
	ax.set_title('Flow Completion Time Comparison GPFC vs PFC:\n' + 
		"condition: static threshold, in total 1000p buffer, 700p for PFC, 500p for GPFC pasue rank 4")
	ax.set_title('Flow Completion Time Comparison GPFC vs PFC:')


	ax.set_xticks(x)
	ax.set_xticklabels(names)
	ax.legend()

	autolabel(rects2,rects1,ax)

	#========================
	#subplot 2, cdf plot
	#========================


	ax2 = axs[1]


	ax2.set_ylabel('CDF')
	ax2.set_xlabel('Time (Microsecond)')
	# ax.set_xticks(cdf_x_1)

	ax2.plot(cdf_x_1, cdf_y_1, '--', color='blue',  label=options.f1)
	ax2.plot(cdf_x_2, cdf_y_2, '--', color='orange',  label=options.f2)
	# ax2.plot(cdf_x_3, cdf_y_3, '--', color='green',  label=options.f3)

	fig.tight_layout()

	# plt.show()

if __name__ == "__main__":


	f = open("result.out", 'w')
	sys.stdout = f
	main()
	f.close()




