#include <Rcpp.h>
#include <iostream>
#include <memory>
using namespace Rcpp;

enum Position {
	root,
	left,
	right};

class Node {
public:
	Node() {
		this->set_depth();
	};
	
	Node(const int value, Node* parent): value_(value) {
		this->parent_ = std::shared_ptr<Node>(parent);
		this->set_depth();
	};
	
	bool is_regular() const {
		return !this->left_ && !this->right_;
	};
	
	bool is_root() const {
		return !this->parent_.lock();
	};
	
	unsigned int depth() const {
		return this->depth_;
	};
private:
	int value_;
	unsigned int depth_;
	std::weak_ptr<Node> parent_;
	std::shared_ptr<Node> left_;
	std::shared_ptr<Node> right_;
	
	Node* set_depth() {
		std::shared_ptr<Node> pa = this->parent_.lock();
		if (pa) {
			this->depth_ = pa->depth_ + 1;
		} else {
			this->depth_ = 0;
		}
		return this;
	};
};

/*
struct Node {
	int value;
	int depth;
	Position pos;
	bool regular;
	
	std::weak_ptr <Node> parent;
	std::shared_ptr <Node> left;
	std::shared_ptr <Node> right;
	
	Node(): depth(0) {};
	
	Node(int value_, int depth_, Position pos_) 
		: value(value_), depth(depth_), pos(pos_), regular(true) {};
	
	std::shared_ptr <Node> child(Position dir) {
		return (dir == Position::left) ? left : right;
	}
};

std::ostream& operator<<(std::ostream& os, const Node& rhs) {
	if (rhs.regular) {
		os << rhs.value;
	} else {
		os << "[" << *rhs.left << "," << *rhs.right << "]";
	}
	return os;
};

class Snailfish {
public:
	Snailfish(List sf) {
		root = std::make_shared<Node>();
		root->pos = Position::root;
		root->regular = false;
		root->depth = 0;
		breed_snailfish(sf, root);
	};
	
	friend std::ostream& operator<<(std::ostream& os, const Snailfish& rhs) {
		os << *rhs.root;
		return os;	
	};
	//private:
	std::shared_ptr<Node> root;
	
	void breed_snailfish(List sf, std::weak_ptr<Node> parent) {
		Rcout << *parent.lock().get() <<":" << parent.lock().use_count() << std::endl;
		if (sf.length() != 2) {
			stop("'sf' must be a list of length 2");
		}
		SEXP left_el  = sf[0],
                    right_el = sf[1];
		SEXPTYPE left_type  = TYPEOF(left_el),
			right_type = TYPEOF(right_el);
		std::shared_ptr<Node> left_node = std::make_shared<Node>();
		left_node->pos = Position::left;
		left_node->parent = parent;
		left_node->regular = false;
		left_node->depth = parent.lock()->depth + 1;
		
		std::shared_ptr<Node> right_node = std::make_shared<Node>();
		right_node->pos = Position::right;
		right_node->parent = parent;
		right_node->regular = false;
		right_node->depth = parent.lock()->depth + 1;
		
		parent.lock()->left = left_node;
		parent.lock()->right = right_node;
		if (left_type == INTSXP) {
			// leaf node
			left_node->value = *INTEGER(left_el);
			left_node->regular = true;
		} else if (left_type == REALSXP){
			left_node->value = (int) *REAL(left_el);
			left_node->regular = true;
		} else if (left_type == VECSXP) {
			breed_snailfish(left_el, left_node); 
		}
		if (right_type == INTSXP) {
			// leaf node
			right_node->value = *INTEGER(right_el);
			right_node->regular = true;
		} else if (right_type == REALSXP){
			right_node->value = (int) *REAL(right_el);
			right_node->regular = true;
		} else if (right_type == VECSXP) {
			breed_snailfish(right_el, right_node); 
		}
	};
	
	std::shared_ptr<Node> next() {
		bool exploded = false;
		return walk(root, exploded);
	};
	
	std::shared_ptr<Node> walk(std::shared_ptr<Node> me, bool &exploded) {
		std::shared_ptr<Node> node, node2;
		if (!exploded) {
			if (me->regular) {
				if (me->value >= 10) {
					node = me;
				}
			} else {
				if (me->depth == 4) {
					exploded = true;
					node = me;
				} else {
					node = walk(me->left, exploded);
					if (!exploded) {
						node2 = walk(me->right, exploded);
					}
					if (!node) {
						node = node2;
					}
				}
			}
		}
		return node;
	};
	
	std::shared_ptr<Node> neighbor(std::shared_ptr<Node> start, Position dir) {
		std::shared_ptr<Node> me = start;
		std::shared_ptr<Node> pa = start->parent.lock();
		while (pa && me->pos == dir) {
			me = pa;
			pa = pa->parent.lock();
		}
		if (pa) {
			Position new_dir = dir;
			while (!pa->regular) {
				pa = pa->child(new_dir);
				new_dir = dir == Position::left ? Position::right : Position::left;
			}
		}
		return pa;
	};
	
	Snailfish* explode(std::shared_ptr<Node> node) {
		std::shared_ptr<Node> left_nb = neighbor(node, Position::left);
		std::shared_ptr<Node> right_nb = neighbor(node, Position::right);
		int left_val  = node->left->value,
			right_val = node->right->value;
		if (left_nb) {
			left_nb->value += left_val;
		}
		if (right_nb) {
			right_nb->value += right_val;
		}
		node->left.reset();
		node->right.reset();
		node->value = 0;
		node->regular = true;
		return this;
	};
	
	Snailfish* split(std::shared_ptr<Node> node) {
		int val = node->value;
		node->left  = std::make_shared<Node>(floor(val / 2.0),
                                       node->depth + 1, Position::left);
		node->right = std::make_shared<Node>(ceil(val / 2.0),
                                       node->depth + 1, Position::right);
		node->left->parent = node;
		node->right->parent = node;
		node->regular = false;
		return this;
	};
	
	Snailfish* reduce() {
		std::shared_ptr<Node> next_node = next();
		while (next_node) {
			if (next_node->regular) {
				split(next_node);
			} else {
				explode(next_node);
			}
			Rcout << *root << std::endl;
			next_node = next();
		}
		return this;
	};
	
	void reset_depth(std::shared_ptr<Node> node) {
		if (node) {
			if (node->parent.lock()) {
				node->depth = node->parent.lock()->depth + 1;
			}
			reset_depth(node->left);
			reset_depth(node->right);
		}
	}
	
	Snailfish& operator+=(const Snailfish& rhs) {
		std::shared_ptr<Node> new_root = std::make_shared<Node>();
		new_root->pos = Position::root;
		
		root->pos = Position::left;
		root->parent = new_root;
		
		rhs.root->pos = Position::right;
		rhs.root->parent = new_root;
		
		new_root->left = root;
		new_root->right = rhs.root;
		
		Rcout << *root << "\t"<< *new_root << std::endl;
		root = new_root;
		reset_depth(root);
		return *this;
	}
	
	friend Snailfish operator+(Snailfish lhs, const Snailfish& rhs) {
		lhs += rhs; // reuse compound assignment
		return lhs; // return the result by value (uses move constructor)
	}
};*/

// [[Rcpp::export]]
void snailfish(List a, List b) {
	Node a_(1, nullptr);
	Node b_(1, &a_);
	Rcout << std::boolalpha << a_.is_regular() << "|" << a_.is_root() << std::endl;
	Rcout << std::boolalpha << b_.is_regular() << "|" << b_.is_root() << std::endl;
	/*Snailfish sf(a);
	Rcout << sf.root->left->left.use_count();
	//Rcout << *sfs.explode(sfs.next()) << std::endl;
	//delete sf;
	//delete sf1;*/
};

/*** R
## Rcpp::sourceCpp(here::here("2021", "Task-18", "Snailfish.cpp"))
library(stringr)
library(dplyr)
l <- "[[[[4,3],4],4],[7,[[8,4],9]]]" %>%
	str_replace_all("\\[", "list(") %>% 
	str_replace_all("\\]", ")") %>% 
	parse(text = .) %>% 
	eval()
k <- list(1L, 1L)
snailfish(l, k)
*/
