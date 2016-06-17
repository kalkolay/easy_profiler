#include "profiler/profiler.h"
#include <fstream>
#include <list>
#include <iostream>
#include <map>
#include <stack>
#include <vector>
#include <iterator>
#include <algorithm> 
#include <ctime>
#include <chrono>

struct BlocksTree
{
	profiler::SerilizedBlock* node;
	std::vector<BlocksTree> children;
	BlocksTree* parent;
	BlocksTree(){
		node = nullptr;
		parent = nullptr;
	}
	BlocksTree(BlocksTree&& that){

		node = that.node;
		parent = that.parent;

		children = std::move(that.children);

		that.node = nullptr;
		that.parent = nullptr;
	}
	~BlocksTree(){
		if (node){
			delete node;
		}
		node = nullptr;
		parent = nullptr;
	}

	BlocksTree(const BlocksTree& rhs)
	{
		copy(rhs);
	}

	BlocksTree& copy(const BlocksTree& rhs)
	{
		if(this == &rhs)
			return *this;
		if(rhs.node)
			node = new profiler::SerilizedBlock(*rhs.node);
		children = rhs.children;
		parent = rhs.parent;
		return *this;
	}

	BlocksTree& operator=(const BlocksTree& rhs)
	{
		return copy(rhs);
	}

	bool operator < (const BlocksTree& other) const 
	{
		if (!node || !other.node){
			return false;
		}
		return node->block()->getBegin() < other.node->block()->getBegin();
	}
};

int main()
{
	std::ifstream inFile("test.prof", std::fstream::binary);

	if (!inFile.is_open()){
		return -1;
	}

	typedef std::map<size_t, BlocksTree> thread_blocks_tree_t;

	thread_blocks_tree_t threaded_trees;

	int blocks_counter = 0;
	auto start = std::chrono::system_clock::now();
	while (!inFile.eof()){
		uint16_t sz = 0;
		inFile.read((char*)&sz, sizeof(sz));
		if (sz == 0)
		{
			inFile.read((char*)&sz, sizeof(sz));
			continue;
		}
		char* data = new char[sz];
		inFile.read((char*)&data[0], sz);
		profiler::BaseBlockData* baseData = (profiler::BaseBlockData*)data;

		BlocksTree& root = threaded_trees[baseData->getThreadId()];

		BlocksTree tree;
		tree.node = new profiler::SerilizedBlock(sz, data);
		blocks_counter++;

		if (root.children.empty()){
			root.children.push_back(std::move(tree));
		}
		else{
			BlocksTree& back = root.children.back();
			auto t1 = back.node->block()->getEnd();
			auto mt0 = tree.node->block()->getBegin();
			if (mt0 < t1)//parent - starts ealier than last ends
			{
				auto lower = std::lower_bound(root.children.begin(), root.children.end(), tree);

				std::move(lower, root.children.end(), std::back_inserter(tree.children));
				
				root.children.erase(lower, root.children.end());

				
			}

			root.children.push_back(std::move(tree));
		}
		

		delete[] data;

	}
	auto end = std::chrono::system_clock::now();

	std::cout << "Blocks count: " << blocks_counter << std::endl;
	std::cout << "dT =  " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
	return 0;
}