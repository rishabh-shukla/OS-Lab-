#include<iostream>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<unistd.h>
#include<queue>
using namespace std;

#define FRAME_SIZE 64
#define FIXED_SIZE 512

struct page_table_content
{
	int page_number; //page frame
	int page_size;
	bool present_bit;
	bool modify_bit;
	int histry_bit;
};

struct page_frames
{
	int number_of_page_frame;
	int *page_size;
	int *page_number;
};

bool memory[512];
struct page_frames *proc;

void create_process(int);
int FIFO(int);
struct page_table_content* create_page_table();
int is_frame_present(struct page_table_content*, int);
int LRU(int);
int LFU(int);
int min_histry_bit(struct page_table_content*);

int main()
{
	cout<<"Enter the number of process(es): ";
	int n;
	cin>>n;
	
	proc = new page_frames[n];
	for (int i=0; i<n; i++)
		create_process(i);
	
	pid_t pid;
	pid = fork();
	for (int i=0; i<n; i++)
	{
		
		if (pid == 0)
		{
			cout<<"The number of page fault by FIFO in Process "<<i+1<<" is "<<FIFO(i)<<".\n";
			cout<<"The number of page fault by LRU in Process "<<i+1<<" is "<<LRU(i)<<".\n";
			cout<<"The number of page fault by LFU in Process "<<i+1<<" is "<<LFU(i)<<".\n\n";
		}
		else
			wait(0);
	}
	return 0;
}

void create_process(int i)
{
	int number_of_page_frame = rand()%150 + 250;
	int arr[4] = {1, 2, 4, 8};
	proc[i].number_of_page_frame = number_of_page_frame;
	proc[i].page_size = new int[number_of_page_frame];
	proc[i].page_number = new int[number_of_page_frame];
	
	for (int j=0; j<number_of_page_frame; j++)
	{
		proc[i].page_size[j] = arr[rand()%4];
		proc[i].page_number[j] = rand()%FIXED_SIZE;
	}
}

int FIFO(int i)
{
	int count = 0;
	queue<int> page_block, empty_block;
	for (int j=0; j<FRAME_SIZE; j++)
		empty_block.push(j);
	int new_page_frame, page_number;
	
	struct page_table_content* page_table = create_page_table();
	for (int j=0; j<proc[i].number_of_page_frame; j++)
	{
		page_number = proc[i].page_number[j];
		if (!is_frame_present(page_table, page_number))
		{
			count++;
			if (empty_block.empty())
			{
				new_page_frame = page_block.front();
				page_block.pop();
				page_block.push(new_page_frame);
				page_table[new_page_frame].page_number = page_number;
				page_table[new_page_frame].page_size = proc[i].page_size[j];
				page_table[new_page_frame].present_bit = true;
			}
			else
			{
				new_page_frame = empty_block.front();
				empty_block.pop();
				page_block.push(new_page_frame);
				page_table[new_page_frame].page_number = page_number;
				page_table[new_page_frame].page_size = proc[i].page_size[j];
				page_table[new_page_frame].present_bit = true;
			}
		}
	}
	return count;
}

struct page_table_content* create_page_table()
{
	struct page_table_content *page_table = new page_table_content[FRAME_SIZE];
	for (int i=0; i<FRAME_SIZE; i++)
	{
		page_table[i].page_number = -1;
		page_table[i].present_bit = false;
	}
	return page_table;
}

int is_frame_present(struct page_table_content* page_table, int page_frame)
{
	for (int i=0; i<FRAME_SIZE; i++)
	{
		if (page_frame == page_table[i].page_number)
			return i+1;
	}
	return 0;
}

int LRU(int i)
{
	int count = 0, page_number, new_page_frame;
	vector<int> page_block;
	queue<int> empty_block;
	for (int j=0; j<FRAME_SIZE; j++)
		empty_block.push(j);
	
	struct page_table_content* page_table = create_page_table();
	
	for (int j=0; j<proc[i].number_of_page_frame; j++)
	{
		page_number = proc[i].page_number[j];
		new_page_frame = is_frame_present(page_table, page_number);
		if (new_page_frame)
		{
			page_block.erase(page_block.begin() + new_page_frame - 1);
			page_block.push_back(page_number);
		}
		else
		{
			count++;
			if (empty_block.empty())
			{
				new_page_frame = page_block.at(0);
				page_block.erase(page_block.begin());
				page_block.push_back(new_page_frame);
				page_table[new_page_frame].page_number = page_number;
				page_table[new_page_frame].page_size = proc[i].page_size[j];
				page_table[new_page_frame].present_bit = true;
			}
			else
			{
				new_page_frame = empty_block.front();
				empty_block.pop();
				page_block.push_back(new_page_frame);
				page_table[new_page_frame].page_number = page_number;
				page_table[new_page_frame].page_size = proc[i].page_size[j];
				page_table[new_page_frame].present_bit = true;
			}
		}
	}
	return count;
}

int LFU(int i)
{
	int count = 0, page_number, new_page_frame;
	queue<int> empty_block;
	for (int j=0; j<FRAME_SIZE; j++)
		empty_block.push(j);
	
	struct page_table_content* page_table = create_page_table();
	
	for (int j=0; j<proc[i].number_of_page_frame; j++)
	{
		page_number = proc[i].page_number[j];
		new_page_frame = is_frame_present(page_table, page_number);
		if (new_page_frame)
			page_table[new_page_frame].histry_bit++;
		else
		{
			count++;
			if (empty_block.empty())
			{
				
				new_page_frame = min_histry_bit(page_table);
				page_table[new_page_frame].page_number = page_number;
				page_table[new_page_frame].page_size = proc[i].page_size[j];
				page_table[new_page_frame].present_bit = true;
				page_table[new_page_frame].histry_bit = 1;
			}
			else
			{
				new_page_frame = empty_block.front();
				empty_block.pop();
				page_table[new_page_frame].page_number = page_number;
				page_table[new_page_frame].page_size = proc[i].page_size[j];
				page_table[new_page_frame].present_bit = true;
				page_table[new_page_frame].histry_bit = 1;
			}
		}
	}
	return count;
}

int min_histry_bit(struct page_table_content* page_table)
{
	int histry_bit=5000, min;
	for (int i=0; i<FRAME_SIZE; i++)
	{
		if (histry_bit > page_table[i].histry_bit)
		{
			histry_bit = page_table[i].histry_bit;
			min = i;
		}
	}
	return min;
}

