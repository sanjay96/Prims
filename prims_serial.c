#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>


struct timeval start, end;
int nodeNum = 0;
int **graphTable;
int **graphTree;
int **ndata;
int maxWeight;
int finishFlag;

int main(int argc, char **argv)
{
	readAdjacencyMatrix();
	prims();
}

int readAdjacencyMatrix() {
	//This function reads the input from the file
	FILE *fp;
	int returnCode =0;
	int temp=0;
	int tempI, tempJ;
	int x,y;
	char *str;
	int lineNum;
	char readChar='-';
	char *bufferLine;
	int countChars = 0; 
	int length = 0;
	int allocSize;
	char *token;
	char *delimiter = "\t";
	
	if((fp=fopen("AdjacencyMatrix.txt","r"))==NULL)
	{
		perror("Error, file cannot be opened.\n");
		exit(1);	
	}
	else
	{
		//First we determine the number of nodes in the graph
		while(readChar!=EOF) 
		{
			readChar = fgetc(fp);
			if(readChar=='\n')  nodeNum++;
		}
		if(nodeNum < numberThreads)
		{	
			numberThreads = nodeNum;
		}
		printf("Nodes found: %d\n", nodeNum);

		//This table will be used to store the initial graph data 
		//extracted from the ipnut file.	
		graphTable=malloc(nodeNum * sizeof(*graphTable));
		for (temp=0; temp<nodeNum; temp++) { graphTable[temp] = malloc(nodeNum * sizeof(int));	}
		
		//This table will be used to store the final graph that will
		//be stored in the file
		graphTree=malloc(nodeNum * sizeof(*graphTree));
		for (temp=0; temp<nodeNum; temp++) { graphTree[temp] = malloc(nodeNum * sizeof(int)); }
			
		allocSize = 3 * nodeNum;
		/*
		We assume that each line will contain two-char node weights (max 99).
		Adding the space delimiter we allocate 3 * nodeNum characters.
		If we run out, we will reallocate space while reading the file.
		*/

		bufferLine = malloc(allocSize * sizeof(char));
		fseek(fp,0,SEEK_SET);
		lineNum=0;
		while(lineNum < nodeNum)
		{
			readChar='-';
			length=0;
			countChars=0;
			while ( (readChar != '\n') && (readChar != EOF) ) 
			{
				if(countChars == (3*nodeNum)) 
				{
					allocSize *= 2;
					countChars = 0;
					bufferLine = realloc(bufferLine, allocSize); // re allocate memory.
				}
				readChar = getc(fp); 
				bufferLine[length] = readChar;
				length++;countChars++;
			}
			graphTable[lineNum][0] = atoi( strtok(bufferLine, delimiter) );

			for (temp=1; temp<nodeNum; temp++) 
			{
				graphTable[lineNum][temp] = atoi( strtok(NULL, delimiter) );
			}
			lineNum++;
		}
		fclose(fp);
		//We determine the maximum weight
		maxWeight=0;
		for(tempI=0;tempI<nodeNum;tempI++)
		{
			for(tempJ=0;tempJ<nodeNum;tempJ++)
			{
				if (graphTable[tempI][tempJ] > maxWeight)
				{
					maxWeight = graphTable[tempI][tempJ];
				}
			}
		}
		printf("\nMaximum weight: %d\n", maxWeight);
	}
	free(bufferLine);
}

int prims()
{
	int tempI,tempJ, temp;
	int returnCode =0;
	ndata=malloc(nodeNum * sizeof(*ndata));
	for (temp=0; temp<nodeNum; temp++) 
	{
		ndata[temp] = malloc(5 * sizeof(int));
	}	
	for(tempI=0;tempI<nodeNum;tempI++)
	{
		for(tempJ=0;tempJ<5;tempJ++)
		{
			if (tempJ==0)
				ndata[tempI][tempJ]=tempI;
			else
				ndata[tempI][tempJ]=0;		
		}
	}
	/*
	The columns used have the following meaning:
	Column 0: The number of the node.
	Column 1: 0 if it is not part of the graph, 1 if it is.
	Column 2: minValue of linkage at this node in the current round.
	Column 3: Node with which minValue is found.
	Column 4: Node that will be its father in the tree.
	*/
	for(tempI=0;tempI<nodeNum;tempI++)
	{
		for(tempJ=0;tempJ<nodeNum;tempJ++)
		{
			graphTree[tempI][tempJ]=0;
		}
	}
	
	gettimeofday(&start,NULL);
	
	int tempCounter1, tempCounter2, tempCounter3;
	int minValue, minNode, minNodeFather;
	int globalMinValue, globalMinNode, globalMinNodeFather;
	int nodesPerThread, extraNodesPerThread;
	int globalWeight = 0;
	int randomNode;
	srand ( time(NULL) );
	randomNode = rand() % nodeNum;
	ndata[randomNode][1]=1;
	finishFlag=0;
	while(!finishFlag)
	{
		for( tempCounter1=0; tempCounter1<nodeNum; tempCounter1++ )
		{
			minValue=maxWeight+1;
			for( tempCounter2=0; tempCounter2<nodeNum; tempCounter2++ )
			{
				if( (graphTable[tempCounter1][tempCounter2] > 0) && (graphTable[tempCounter1][tempCounter2] < minValue) && (ndata[tempCounter2][1]==0))
				{
					minValue = graphTable[tempCounter1][tempCounter2];
					minNode = tempCounter2;
					minNodeFather = tempCounter1;
				}
			}
			if ( minValue < (maxWeight+1) )
			{
				ndata[tempCounter1][2] = minValue;
				ndata[tempCounter1][3] = minNode;
				ndata[tempCounter1][4] = minNodeFather;
			}
		}
		tempCounter2=0;
		globalMinValue = maxWeight+1;
			for( tempCounter1=0; tempCounter1<nodeNum; tempCounter1++ )
			{
				if(ndata[tempCounter1][1]==1)
				{
					tempCounter2++;					
					if ((ndata[tempCounter1][2] < globalMinValue) && (ndata[tempCounter1][2] > 0))
					{
						globalMinValue = ndata[tempCounter1][2];
						globalMinNode = ndata[tempCounter1][3];
						globalMinNodeFather = ndata[tempCounter1][4];
						
						ndata[tempCounter1][2] = 0;
					}
				}
			}
			if(globalMinValue < maxWeight+1)
			{
				ndata[globalMinNode][1] = 1;
				tempCounter2++;
				globalWeight += globalMinValue;
				graphTree[globalMinNode][globalMinNodeFather]=1;
				graphTree[globalMinNodeFather][globalMinNode]=1;
			}
			if(tempCounter2 == nodeNum)
			{
				finishFlag=1;
			}
	}	
	printf("GlobalWeight: %d\n", globalWeight);
	gettimeofday(&end,NULL);
	printf("Calculation complete.\n");
	printf("Total computation time: %ld microsecs.\n", ((end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec)));
}
