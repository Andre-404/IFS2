#include "issueTracker.h"


void issueTracker::addIssue(Token token, string msg, file* src) {
	issues.push_back(issue(token, msg, src));
}

void issueTracker::printIssues() {
	const string cyan = "\u001b[38;5;117m";
	const string black = "\u001b[0m";
	const string red = "\u001b[38;5;196m";
	const string yellow = "\u001b[38;5;220m";
	
	//the issues array contains preprocessor, parser and compiler errors
	for (issue cur : issues) {
		report(cur.src, cur.token, cur.msg);
	}

	std::cout << "Detected " << red << issues.size() << black + " errors\n";
}