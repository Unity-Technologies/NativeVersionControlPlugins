#include <iostream>
#include <string>
#include "CrudeSax.h"

using namespace std;
using namespace crudesax;

class TestParser : public SaxParser
{
public:
	virtual void OnElementStart(const std::string& tagName, const Attributes& attribs) 
	{
		elmName = tagName;
		attrs = attribs;
	}
	virtual void OnElementEnd(const std::string& tagName) {}; // e.g. </foo>
	virtual void OnText(const std::string& text) {}; //e.g. the 'bla' in <foo a="fd">bla</foo>
	virtual void OnComment(const std::string& text) {}; // e.g. <!-- foobar -->
	virtual void OnProcessingInstruction(const std::string& text) {}; //  e.g. <?xml ...>

	void ClearedFeed(const string& s) 
	{
		ClearAll();
		Feed(s);
	}

	void Clear() { elmName.clear(); attrs.clear(); }
	void ClearAll() { Clear(); m_Buffer.clear(); m_Depth = 0; }
	string elmName;
	Attributes attrs;
} parser;

ostream& operator<<(ostream& os, const Attributes& a)
{
	string delim;
	for (Attributes::const_iterator i = a.begin(); i != a.end(); ++i)
	{
		os << delim << i->first << "=" << i->second;
		delim = ", ";
	}
	return os;
}

Attributes attrs(const std::string& name, const string& value)
{
	Attributes a;
	a[name] = value;
	return a;
}

Attributes attrs(const std::string& name1, const string& value1, 
				 const std::string& name2, const string& value2)
{
	Attributes a;
	a[name1] = value1;
	a[name2] = value2;
	return a;
}

Attributes attrs(const std::string& name1, const string& value1, 
				 const std::string& name2, const string& value2,
				 const std::string& name3, const string& value3)
{
	Attributes a;
	a[name1] = value1;
	a[name2] = value2;
	a[name3] = value3;
	return a;
}


void Equal(const char* description, const char* expectedName, const Attributes& expectedAttrs)
{
	bool okName = parser.elmName == expectedName;
	bool okAttrs = parser.attrs == expectedAttrs;
	cout << description << ": " << (okName && okAttrs? "OK" : "Failed") << endl;
	if (! (okName && okAttrs) )
	{
		if (!okName)
		{
			cout << "\tResultName: " << parser.elmName << endl;
			cout << "\tExpectName: " << expectedName << endl;
		}
		if (!okAttrs)
		{
			cout << "\tResultAttr: " << parser.attrs << endl;
			cout << "\tExpectAttr: " << expectedAttrs << endl;
		}
	}
}

int main(int argc, char* argv[])
{
	Attributes noattrs;

	Equal("Empty feed gived no tags", "", noattrs);

	parser.Feed("<foo>");
	Equal("Element with no attrs", "foo", noattrs);

	parser.ClearedFeed("<fo");
	Equal("Partial element name gives no name", "", noattrs);

	parser.Feed(">");
	Equal("Ending partial name gives a name", "fo", noattrs);

	parser.ClearedFeed("<bar  >");
	Equal("Element name with ending space", "bar", noattrs);

	parser.ClearedFeed("  <bar>");
	Equal("Element name with space before", "bar", noattrs);

	parser.ClearedFeed("<foo a1>");
	Equal("Element with non valued attr", "bar", attrs("a1",""));

	return 0;
}
