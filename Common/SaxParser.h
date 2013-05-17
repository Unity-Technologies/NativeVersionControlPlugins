#include <string>
#include <map>
#include <exception>

namespace crudesax
{

class SaxParserException : public std::exception
{
public:
	SaxParserException(const std::string& _about, size_t lineNum);
	~SaxParserException() throw();
	virtual const char* what() const throw();
	std::string about;
	size_t lineNumber;
};

typedef std::map<std::string, std::string> Attributes;

class SaxParser
{
public:
	SaxParser();
	void Feed(const std::string& data);

	virtual void OnElementStart(const std::string& tagName, const Attributes& attribs) {}; // e.g. <foo a="fd">
	virtual void OnElementEnd(const std::string& tagName) {}; // e.g. </foo>
	virtual void OnText(const std::string& text) {}; //e.g. the 'bla' in <foo a="fd">bla</foo>
	virtual void OnComment(const std::string& text) {}; // e.g. <!-- foobar -->
	virtual void OnProcessingInstruction(const std::string& text) {}; //  e.g. <?xml ...>
	
protected:
	std::string m_Buffer;
	size_t m_Depth;
};


} // end namespace crudesax
