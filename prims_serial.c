#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

struct timeval start, end; 
int numNode = 0; /*Total number of nodes*/ 
int randNode; /* Root node of the generated tree */
int **graphTable; /* Table that will store the data in the file. */
int **graphTree; /* This table will hold the constructed tree. */
int **ndata; /*This table will hold data generated during the calculations. */
int maxWeight; /*This is the maximum linkage value (weight) found in the original graph. */
int globalWeight; /* The total sum of all the linkages in the generated tree. */
int finish;
int numberThreads;

int main(int argc, char **argv)
{
	numberThreads = 10;
	readAdjacencyMatrix();	
	prims();		
	return 0;
}

int readAdjacencyMatrix() {
	//This Function reads the data from the input file	
	FILE *fp;
	int temp=0,tempI, tempJ,x,y;
	char *str;
	int lineNum;
	char readChar='-';
	char *bufferLine;
	long int countChars = 0; 
	int length = 0;
	int allocSize;
	char *token;
	char *delimiter = "\t";
	if((fp=fopen("AdjacencyMatrix.txt","r"))==NULL)
	{
		perror("Can't open file\n");
		exit(1);
	}
	else
	{
		//We need to find the number of nodes in the graph
		while(readChar!=EOF) 
		{
			readChar = fgetc(fp);
			if(readChar=='\n') { numNode++; }
		}
		if(numNode < numberThreads) { numberThreads = numNode; }
		printf("Nodes found: %d Threads used: %d\n", numNode, numberThreads);
		
		//This table is used to store the original values of the graph.
		graphTable=malloc(numNode * sizeof(int *));
		for (temp=0; temp<numNode; temp++) { graphTable[temp] = malloc(numNode * sizeof(int)); }

		//This table will be used to store the final graph stored in the file
		graphTree=malloc(numNode * sizeof(int *));
		for (temp=0; temp<numNode; temp++) { graphTree[temp] = malloc(numNode * sizeof(int)); }
			
		allocSize = 10 * numNode;
		bufferLine = malloc(allocSize * sizeof(char));
		fseek(fp,0,SEEK_SET);
		lineNum=0;

		/* Now we return to the start of the file in order to populate the table with the graph data. */
		while(lineNum < numNode)
		{
			readChar='-'; length=0; countChars=0;
			while ( (readChar != '\n') && (readChar != EOF) ) 
			{
				if(countChars == (10*numNode)) 
				/* if line is longer than expected, expanding the buffer.Double the available buffer size. */
				{ 
					allocSize *= 2;
					countChars = 0;
					bufferLine = realloc(bufferLine, allocSize);
				}
				readChar = getc(fp);
				bufferLine[length] = readChar;
				length++;countChars++;
			}
			graphTable[lineNum][0] = atoi(strtok(bufferLine, delimiter) );
			for (temp=1; temp<numNode; temp++) 
			{
				graphTable[lineNum][temp] = atoi(strtok(NULL, delimiter) );
			}
			lineNum++;
		}
		fclose(fp);

		// Calculating the max Weight of the linkages
		maxWeight=0;
		for(tempI=0;tempI<numNode;tempI++)
		{
			for(tempJ=0;tempJ<numNode;tempJ++)
			{
				if (graphTable[tempI][tempJ] > maxWeight)
				{
					maxWeight = graphTable[tempI][tempJ];
				}
			}
		}
		printf("Maximum weight: %d\n", maxWeight);
	}
	free(bufferLine);
}

int prims()
{
	int tempI,tempJ, temp;
	int rowsThread, extrarowsThread;
	int minValue,minNode,minNodeFather;
	ndata=malloc(numNode * sizeof(*ndata));
	for (temp=0; temp<numNode; temp++) { ndata[temp] = malloc(5 * sizeof(int)); }
	/*This table will be used during the calculations to hold temporary data. */			
	for(tempI=0;tempI<numNode;tempI++)
	{
		for(tempJ=0;tempJ<5;tempJ++)
		{
			if (tempJ==0)
				ndata[tempI][tempJ]=tempI;
			else
				ndata[tempI][tempJ]=0;
		}
	}
	/*The columns used have the following meaning:
	Column 0: The number of the node.
	Column 1: 0 if it is not part of the graph, 1 if it is.
	Column 2: minValue of linkage at this node in the current round.
	Column 3: Node with which minValue is found.
	Column 4: Node that will be its father in the tree.
	*/
	for(tempI=0;tempI<numNode;tempI++)
		for(tempJ=0;tempJ<numNode;tempJ++)
			graphTree[tempI][tempJ]=0;
	
	gettimeofday(&start,NULL);
	int tempCounter1, tempCounter2, tempCounter3;
	int globalMinValue, globalMinNode, globalMinNodeFather;

	finish=0;
	srand ( time(NULL) );
	randNode = rand() % numNode;
	ndata[randNode][1]=1;
	while(!finish)
	{
		for( tempCounter1=0; tempCounter1<numNode; tempCounter1++ )
		{
			/*Every thread scans the nodes assigned to it calculating an optimal linkage for each node*/
			ndata[tempCounter1][2]=maxWeight+1;
			for( tempCounter2=0; tempCounter2<numNode; tempCounter2++ )
			{
				/*Scan the current node in the graph Table for the minimum linkage value.
				  The linkages that will be tested will notalready belong to the tree.
				*/
				minValue=maxWeight+1;
				if( (graphTable[tempCounter1][tempCounter2] > 0) && (graphTable[tempCounter1][tempCounter2] < ndata[tempCounter1][2]) && (ndata[tempCounter2][1]==0))
				{
					/* If this linkage has optimal weight and is not already in the tree*/
					minValue = graphTable[tempCounter1][tempCounter2];
					minNode = tempCounter2;
					minNodeFather = tempCounter1;
				}
			}
			if ( minValue < (maxWeight+1) )
			{
				/*
				A new minimum linkage has been discovered for this node.
				Write it to the nodeData table to be compared with the rest.
				*/
				ndata[tempCounter1][2] = minValue;
				ndata[tempCounter1][3] = minNode;
				ndata[tempCounter1][4] = minNodeFather;
			}
		}

		tempCounter2=0;
		globalMinValue = maxWeight+1;

		for( tempCounter1=0; tempCounter1<numNode; tempCounter1++ )
		{
			if(ndata[tempCounter1][1]==1)
			{
				tempCounter2++;									
				if ((ndata[tempCounter1][2] < globalMinValue) && (ndata[tempCounter1][2] > 0))
				{
					/*	If the minimum distance column contains a positive value
					compare it with to rest to find the minimum.After the check 
					reset it to 0 for the next round.
					*/
					globalMinValue = ndata[tempCounter1][2];
					globalMinNode = ndata[tempCounter1][3];
					globalMinNodeFather = ndata[tempCounter1][4];
					ndata[tempCounter1][2] = 0;
				}
			}
		}
		if(globalMinValue < maxWeight+1)
		{
			/*Add the node with the best linkage to the spanning tree.*/
			ndata[globalMinNode][1] = 1;
			tempCounter2++;
			globalWeight += globalMinValue;
			graphTree[globalMinNode][globalMinNodeFather]=1;
			graphTree[globalMinNodeFather][globalMinNode]=1;
		}
		if(tempCounter2 == numNode)
		{
			finish=1;
			printf("GlobalWeight: %d\n", globalWeight);
		}
	}
	gettimeofday(&end,NULL);
	printf("Calculation complete.\n");
	printf("Total computation time: %4.3f milliseconds.\n", (double)(((double)(end.tv_sec * 1000000 + end.tv_usec) - (double)(start.tv_sec * 1000000 + start.tv_usec))/1000));	
}
