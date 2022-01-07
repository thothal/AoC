#include <Rcpp.h>
#include <iostream>
using namespace Rcpp;


enum Position {
	root,
	left,
	right};

struct Node {
	int value;
	int depth;
	Position pos;
	Node *parent;
	Node *left;
	Node *right;
	bool regular;
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
		root = new Node;
		root->left = 0;
		root->right = 0;
		root->pos = Position::root;
		root->regular = false;
		root->depth = 0;
		breed_snailfish(sf, root);
	};
	
	~Snailfish() {
		kill_snailfish(root);
	};
	
	friend std::ostream& operator<<(std::ostream& os, const Snailfish& rhs) {
		os << *rhs.root;
		return os;	
	};
private:
	Node *root = 0;
	
	void breed_snailfish(List sf, Node *parent) {
		if (sf.length() != 2) {
			stop("'sf' must be a list of length 2");
		}
		SEXP left_el  = sf[0],
           right_el = sf[1];
		SEXPTYPE left_type  = TYPEOF(left_el),
			      right_type = TYPEOF(right_el);
		Node *left_node = new Node;
		left_node->left = 0;
		left_node->right = 0;
		left_node->pos = Position::left;
		left_node->parent = parent;
		left_node->regular = false;
		left_node->depth = parent->depth + 1;
		
		Node *right_node = new Node;
		right_node->left = 0;
		right_node->right = 0;
		right_node->pos = Position::right;
		right_node->parent = parent;
		right_node->regular = false;
		right_node->depth = parent->depth + 1;
		
		parent->left = left_node;
		parent->right = right_node;
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
	
	void kill_snailfish(Node *node) {
		if (node != 0) {
			kill_snailfish(node->left);
			kill_snailfish(node->right);
			delete node;
		}
	};
};

// [[Rcpp::export]]
void snailfish(List a) {
	Snailfish *i = new Snailfish(a);
	Rcout << *i;
	delete i;
};

/*** R
snailfish(list(1L, 2L))
*/
