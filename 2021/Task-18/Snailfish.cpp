#include <Rcpp.h>
#include <iostream>
#include <memory>
using namespace Rcpp;

enum Position {
	left,
	right,
	root
};

class Node: public std::enable_shared_from_this<Node> {
public:
	
	[[nodiscard]] static std::shared_ptr<Node> create() {
		return std::shared_ptr<Node>(new Node());
	};
	
	bool is_regular() const {
		return !this->left_ && !this->right_;
	};
	
	bool is_root() const {
		return !this->parent_.lock();
	};
	
	std::shared_ptr<Node> ptr() {
		return shared_from_this();
	};
	
	unsigned int depth() {
		unsigned int depth = 0;
		std::shared_ptr<Node> pa = this->parent_.lock();
		while (pa) {
			depth++;
			pa = pa->parent_.lock();
		}
		return depth;
	}
	
	int value() const {
		return this->value_;
	};
	
	Position position() {
		if (!this->is_root()) {
			std::shared_ptr<Node> pa = this->parent_.lock();
			if (this->ptr() == pa->left_) {
				return Position::left;
			} else {
				return Position::right;
			}
		} else {
			return Position::root;
		}
	};
	
	long magnitude() {
		long res = 0;
		if (this->is_regular()) {
			res = this->value_;
		} else {
			res = 3 * this->left_->magnitude() + 2 * this->right_->magnitude();
		}	
		return res;
	};
	
	std::shared_ptr<Node> set_value(const int value) {
		this->value_ = value;
		return this->ptr();
	};
	
	std::shared_ptr<Node> set_child(std::shared_ptr<Node> kid, const Position pos) {
		kid->parent_ = this->ptr();
		if (pos == Position::left) {
			this->left_ = kid;
		} else {
			this->right_ = kid;
		}
		return this->ptr();
	};
	
	std::shared_ptr<Node> set_childs(const std::array<std::shared_ptr<Node>, 2>& kids) {
		this->set_child(kids[0], Position::left);
		this->set_child(kids[1], Position::right);
		return this->ptr();
	};
	
	std::shared_ptr<Node> next() {
		bool exploded = false;
		return this->next(exploded);
	};
	
	std::shared_ptr<Node> explode() {
		std::shared_ptr<Node> left_nb = neighbor(Position::left);
		std::shared_ptr<Node> right_nb = neighbor(Position::right);
		int left_val = this->left_->value_,
			right_val = this->right_->value_;
		if (left_nb) {
			left_nb->value_ += left_val;
		}
		if (right_nb) {
			right_nb->value_ += right_val;
		}
		this->left_.reset();
		this->right_.reset();
		this->value_ = 0;
		return this->ptr();
	};
	
	std::shared_ptr<Node> split() {
		int val = this->value_;
		this->left_ = Node::create();
		this->left_->set_value(floor(val / 2.0));
		this->right_ = Node::create();
		this->right_->set_value(ceil(val / 2.0));
		return this->ptr();
	};
	
	
	
	friend std::ostream& operator<<(std::ostream& os, const Node& rhs);
private:
	// Do not allow direct init to force use of create
	Node() = default;
	int value_;
	std::weak_ptr<Node> parent_;
	std::shared_ptr<Node> left_;
	std::shared_ptr<Node> right_;
	
	std::shared_ptr<Node> neighbor(const Position dir) {
		std::shared_ptr<Node> me = this->ptr();
		std::shared_ptr<Node> pa = this->parent_.lock();
		while (pa && me->position() == dir) {
			me = pa;
			pa = pa->parent_.lock();
		}
		if (pa) {
			Position new_dir = dir;
			while (!pa->is_regular()) {
				if (new_dir == Position::left) {
					pa = pa->left_;
				} else {
					pa = pa->right_;
				}
				new_dir = dir == Position::left ? Position::right : Position::left;
			}
		}
		return pa;
	};
	
	std::shared_ptr<Node> next(bool& exploded) {
		std::shared_ptr<Node> node, node2;
		if (!exploded) {
			if (this->is_regular()) {
				if (this->value() >= 10) {
					node = this->ptr();
				}
			} else {
				if (this->depth() == 4) {
					exploded = true;
					node = this->ptr();
				} else {
					node = this->left_->next(exploded);
					if (!exploded) {
						node2 = this->right_->next(exploded);
						if (!node || exploded) {
							node = node2;
						}
					}
				}
			}
		}
		return node;
	};
};

std::ostream& operator<<(std::ostream& os, const Node& rhs) {
	if (rhs.is_regular()) {
		os << rhs.value();
	} else {
		os << "[" << *rhs.left_ << "," << *rhs.right_ << "]";
	}
	return os;
};

class Snailfish {
public:
	Snailfish* breed_snailfish(List sf) {
		this->root_ = Node::create();
		this->breed_snailfish(sf, this->root_);
		return this;
	};
	
	Snailfish* reduce(bool verbose = false) {
		std::shared_ptr<Node> next_node = this->root_->next();
		while (next_node) {
			if (next_node->is_regular()) {
				if (verbose) {
					Rcout << "after splitting @" << *next_node << ":\t\t";	
				}
				next_node->split();
				if (verbose) {
					Rcout << *this << std::endl;
				}
			} else {
				if (verbose) {
					Rcout << "after exploding @" << *next_node << "\t\t";
				}
				next_node->explode();
				if (verbose) {
					Rcout << *this << std::endl;
				}
			}
			next_node = this->root_->next();
		}
		return this;	
	};
	
	Snailfish& operator+=(const Snailfish& rhs) {
		std::shared_ptr<Node> new_root = Node::create();
		std::array<std::shared_ptr<Node>, 2> kids = {this->root_, rhs.root_};
		new_root->set_childs(kids);
		this->root_ = new_root;
		return *this;
	}
	
	long magnitude() {
		return this->root_->magnitude();
	}
	
	friend std::ostream& operator<<(std::ostream& os, const Snailfish& rhs);
	
private:
	std::shared_ptr<Node> root_;
	
	void breed_snailfish(List sf, std::shared_ptr<Node> parent) {
		if (sf.length() != 2) {
			stop("'sf' must be a list of length 2");
		}
		SEXP left_el = sf[0],
                   right_el = sf[1];
		SEXPTYPE left_type  = TYPEOF(left_el),
			right_type = TYPEOF(right_el);
		std::shared_ptr<Node> left_node = Node::create();
		std::shared_ptr<Node> right_node = Node::create();
		
		if (left_type == INTSXP) {
			// leaf node
			left_node->set_value(*INTEGER(left_el));
		} else if (left_type == REALSXP){
			left_node->set_value((int) *REAL(left_el));
		} else if (left_type == VECSXP) {
			breed_snailfish(left_el, left_node); 
		}
		
		if (right_type == INTSXP) {
			// leaf node
			right_node->set_value(*INTEGER(right_el));
		} else if (right_type == REALSXP){
			right_node->set_value((int) *REAL(right_el));
		} else if (right_type == VECSXP) {
			breed_snailfish(right_el, right_node); 
		}		
		std::array<std::shared_ptr<Node>, 2> kids = {left_node, right_node};
		parent->set_childs(kids);
	};
};

std::ostream& operator<<(std::ostream& os, const Snailfish& rhs) {
	os << *rhs.root_;
	return os;
};

Snailfish operator+(Snailfish lhs, const Snailfish& rhs) {
	lhs += rhs;
	return lhs;
}

// [[Rcpp::export]]
void add_snailfish(const List& sfs) {
	Snailfish sf0, sfi;
	sf0.breed_snailfish(sfs[0]);
	for (auto i = 1; i < 3; i++) {//sfs.length(); i++) {
		sfi.breed_snailfish(sfs[i]);
		sf0 = sf0 + sfi;
		sf0.reduce(true);
		Rcout << sf0 << std::endl;
	}
	Rcout << sf0.magnitude() << std::endl;
};


/*** R
## Rcpp::sourceCpp(here::here("2021", "Task-18", "Snailfish.cpp"))
library(stringr)
library(dplyr)
library(purrr)
l <- "[[[0,[4,5]],[0,0]],[[[4,5],[2,6]],[9,5]]]
[7,[[[3,7],[4,3]],[[6,3],[8,8]]]]
[[2,[[0,8],[3,4]]],[[[6,7],1],[7,[1,6]]]]
[[[[2,4],7],[6,[0,5]]],[[[6,8],[2,8]],[[2,1],[4,5]]]]
[7,[5,[[3,8],[1,4]]]]
[[2,[2,2]],[8,[8,1]]]
[2,9]
[1,[[[9,3],9],[[9,0],[0,7]]]]
[[[5,[7,4]],7],1]
[[[[4,2],2],6],[8,7]]" %>%
	str_split("\n") %>% 
	`[[`(1L) %>% 
	str_replace_all("\\[", "list(") %>% 
	str_replace_all("\\]", ")") %>%
	map(~ parse(text = .x) %>% 
		 	eval())
add_snailfish(l)
*/
