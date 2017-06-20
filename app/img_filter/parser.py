import re

lines = [line.rstrip('\n') for line in open('../../usr/sim/mpsoc_sim/reports/out1.txt')]
#print lines
string = ''
for line in lines:
	string += line
	string += '\n'

m = re.search('{CODE}(.*){CODE}', string, re.DOTALL)
if m:
	#print m.group(1)
	target = open('filter_image.h', 'w')
	target.write(m.group(1))
	target.close()
else:
	print "Problema na regex"