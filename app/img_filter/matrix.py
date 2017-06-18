def matrix_print(a, h, w) :
	print "matrix is a %dx%d" % (h, w)
	i = 0
	while (i < h) :
		j = 0
		while (j < w) :
			print "%d\t" % a[i * w + j],
			j += 1
		print ""
		i += 1

def matrix_print_sub(a, h, w, k, l, bh, bw) :
	print "sub matrix is a %dx%d, line %d column %d" % (bh, bw, k, l)
	i = 0
	while (i < bh) :
		j = 0
		while (j < bw) :
			pos = (i + k * bh) * w + (j + l * bw)
			#print "%d\t" % pos,
			#print "position %d" % (pos)
			if (pos-(w*i) > w-1): # padding lateral, FIXME: TRATAR PADDING vertical
				print "0\t",
			#elif pos > (w*h):
				#print "0\t",
			else:
				print "%d\t" % a[pos],
			j += 1
		print ""
		i += 1

def matrix_print_sub2(a, h, w, k, l, bh, bw) :
	i = 0
	start_line = (k*bh)
	start_column =  (l * bw)
	print "Block start line %d start column %d" % (start_line, start_column)
	while (i < bh) :
		j = 0			
		while (j < bw) :
			#print "Line %d Column %d" % ((i+start_line), (j+start_column))
			if i+start_line > h-1 or j+start_column > w-1:
				print "0\t",
			else:
				print a[i+start_line][j+start_column],
			j += 1

		print ""
		i += 1

def main() :
	# array (representing a 2d matrix)
	m = [
		[11, 12, 13, 14, 15, 16, 17, 18],
		[21, 22, 23, 24, 25, 26, 27, 28],
		[31, 32, 33, 34, 35, 36, 37, 38],
		[41, 42, 43, 44, 45, 46, 47, 48],
		[51, 52, 53, 54, 55, 56, 57, 58],
		[61, 62, 63, 64, 65, 66, 67, 68],
		[71, 72, 73, 74, 75, 76, 77, 78],
		[81, 82, 83, 84, 85, 86, 87, 88],
		[91, 92, 93, 94, 95, 96, 97, 98]
	]

	w = 5
	h = 6
	matrix = [[0 for x in range(w)] for y in range(h)] 

	vector = [11, 12, 13, 14, 15,
			  21, 22, 23, 24, 25,
			  31, 32, 33, 34, 35,
			  41, 42, 43, 44, 45,
			  51, 52, 53, 54, 55,
			  61, 62, 63, 64, 65]

	

	pos = 0
	for i in range(h):
		for j in range(w):
			matrix[i][j] = vector[pos]
			pos += 1

	print matrix


	#for linha in range(9):
#		for coluna in range(8):
#			print "%d\t"%m[linha][coluna],
		#print ""
	# print the matrix
	#matrix_print(m, 9, 8)
	# matrix, height, width, block line, block column, block height, block width
	#matrix_print_sub2(m, 9, 8, 1, 1, 5, 5)

if __name__ == "__main__" : main()
