# Open the file for reading
with open('combine.txt', 'r') as file:

    # Initialize variables for the column sums
    column1_sum = 0
    column2_sum = 0

    # Loop through each line in the file
    for line in file:
        if line == "\n" :
            continue
        # Split the line into two columns of numbers
        print (line)
        columns = line.split()

        # Add the numbers from each column to its respective sum
        column1_sum += float(columns[0])
        column2_sum += float(columns[1])

    # Divide the sum of column 1 by column 2 and print the result
    result = column1_sum / column2_sum
    print("Hit Rate:", result)
