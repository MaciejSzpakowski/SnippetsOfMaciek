#include <iostream>
#include <iterator>
#include <algorithm>

template <class T>
struct list_node
{
	T data;
	list_node<T>* next;
};

template <class T>
class llist
{
public:	

	class iterator : public std::iterator<std::input_iterator_tag, list_node<T>>
	{
		list_node<T>* p;
	public:
		iterator(list_node<T>* x) :p(x) {}
		iterator(const iterator& mit) : p(mit.p) {}
		iterator& operator++() { p = p->next; return *this; }
		iterator operator++(int) { iterator tmp(*this); operator++(); return tmp; }
		bool operator==(const iterator& rhs) { return p == rhs.p; }
		bool operator!=(const iterator& rhs) { return p != rhs.p; }
		T& operator*() { return p->data; }
	};

	list_node<T>* head;

	llist()
	{
		head = nullptr;
	}

	iterator begin()
	{
		return iterator(head);
	}

	iterator end()
	{
		return iterator(nullptr);
	}

	void insert_front(T&& data)
	{
		list_node<T>* new_node = new list_node<T>;
		new_node->data = data;
		new_node->next = head;
		head = new_node;
	}
};

int main(int argc, char* argv[])
{
	llist<int> list;
	list.insert_front(3);
	list.insert_front(5);
	list.insert_front(7);
	list.insert_front(9);

	auto r = std::find(list.begin(), list.end(), 13);
	if(r != list.end())
		printf("%i\n", *r);
	else
		printf("Not found\n");

	return 0;
}
