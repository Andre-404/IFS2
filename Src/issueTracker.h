#pragma once
#include "common.h"
#include "scanner.h"

struct issue {
	Token token;
	string msg;
	file* src;

	issue(Token _token, string _msg, file* _src) : token(_token), msg(_msg), src(_src) {};
};

class issueTracker {
private:
	vector<issue> issues;
public:
	void printIssues();
	void addIssue(Token token, string msg, file* src);
};