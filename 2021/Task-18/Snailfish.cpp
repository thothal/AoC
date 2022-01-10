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

	Node* explode() {
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
		return this;
	};
	
	Node* split() {
		int val = this->value_;
		this->left_ = Node::create();
		this->left_->set_value(floor(val / 2.0));
		this->right_ = Node::create();
		this->right_->set_value(ceil(val / 2.0));
		return this;
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
		std::shared_ptr<Node> node;
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
					if (!exploded && !node) {
						node = this->right_->next(exploded);
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
	
	std::shared_ptr<Node> root() {
		return root_;
	}
	
	Snailfish* reduce() {
		std::shared_ptr<Node> next_node = this->root_->next();
		while (next_node) {
			if (next_node->is_regular()) {
				next_node->split();
			} else {
				next_node->explode();
			}
			Rcout << *root_ << std::endl;
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

/*

 void reset_depth(std::shared_ptr<Node> node) {
 if (node) {
 if (node->parent.lock()) {
 node->depth = node->parent.lock()->depth + 1;
 }
 reset_depth(node->left);
 reset_depth(node->right);
 }
 }
 
 */

// [[Rcpp::export]]
void snailfish(const List& a, const List& b) {
	Snailfish sf1, sf2;
	sf1.breed_snailfish(a);
	sf2.breed_snailfish(b);
	Rcout << *(sf1 + sf2).reduce() << std::endl;
	//Rcout << *sfs.explode(sfs.next()) << std::endl;
	//delete sf;
	//delete sf1;
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
