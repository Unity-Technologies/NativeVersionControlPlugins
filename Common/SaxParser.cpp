#include "CrudeSax.h"
#include <sstream>

namespace crudesax
{

SaxParserException::SaxParserException(const string& _about, size_t lineNum)
	: about(_about), lineNumber(lineNum) {}

SaxParserException::~SaxParserException() throw()
{
}

const char* SaxParserException::what() const throw()
{
	static string buf;
	stringstream ss;
	ss << about << " (line " << lineNumber << ")";
	buf = ss.str();
	return buf.c_str();
}

enum parse_state_t
{
	PS_NonText,
	
};

class ParseState
{
public:
	ParseState(SaxParser& p, string& _data) : parser(p), data(_data), len(data.length()), lineIdx(0) {}

	bool SkipUntilNot(const char* chars)
	{
		string::size_type i;

		do 
		{
			i = data.find_first_not_of(chars, cur);

			if (i == string::npos) return false;

			cur = i;

			if (data[i] == '\n')
			{
				line++, i++;
				lineIdx = i;
			}
			else if (data[i] == '\r')
				i++;
			else
				break;

			if (i == len) return false;

		} while (true);
		
		col = cur - lineIdx;
	}

	bool SkipUntil(const char* chars)
	{
		string::size_type i;
		string match(chars);
		match += "\r\n";
		do 
		{
			i = data.find_first_of(chars, cur);

			if (i == string::npos) return false;

			cur = i;

			if (data[i] == '\n')
			{
				line++, i++;
				lineIdx = i;
			}
			else if (data[i] == '\r')
				i++;
			else
				break;

			if (i == len) return false;

		} while (true);
		
		col = cur - lineIdx;
	}

	bool SkipWhitespace()
	{
		return SkipUntilNot(" \t");
	}

	bool SkipUntilWhitespace()
	{
		return SkipUntil(" \t");
	}

	bool SkipUntil(char c)
	{
		char buf[2];
		buf[0] = c;
		buf[1] = 0x00;
		return SkipUntil(buf);
	}
	
	char Cur() const { return data[cur]; }
	string::size_type Idx() const { return cur; }

	bool Skip() 
	{
		cur++;
		if (cur == len)
			return false;
	}

	// Remove all chars until and including Cur()
	void eat() 
	{
		if (cur >= len - 1)
			data.clear();
		else
			data = data.substr(0, cur + 1);
	}

	string Slice(string::size_type begin, string::size_type end)
	{
		return data.substr(begin, end - begin);
	}

	string Slice(string::size_type begin)
	{
		return data.substr(begin, cur - begin);
	}

	SaxParser& parser;
	string::size_type cur;
	string& data;
	string::size_type len;
	
	int line;
	string::size_type col;
	string::size_type lineIdx;
};
	

SaxParser::SaxParser() : m_Depth(0)
{ 
	
}

static void ParseElement(ParserState& state, const string& name);
static void ParseProcessingInstruction(ParserState& state, const string& name);
static void ParseComment(ParserState& state);

// Parse non-text nodes and modify data by removing text used
// Returns false if more data is needed
// Returns true if we should call again
static bool ParseNonText(ParserState& state)
{
	if (!state.SkipWhitespace()) return false; // keep looking when more data is available

	if (state.Cur() != '<') throw SaxParserException(state.line, "No starting < found");
	if (state.Skip()) return false;

	if (!state.SkipWhitespace()) return false; 
	string::size_type nameBeginIdx = state.Idx();

	if (!state.SkipUntil(" \t/>")) return false;
	string elementName = data.slice(nameBeginIdx);

	if (!state.SkipWhitespace()) return false; 
	
	// Check for <foo/   >  case
	if (state.Cur() == '/')
	{
		if (!state.Skip() || !state.SkipWhitespace()) return false;
	}

	if (state.Cur() == '>')
	{
		// This is a simple <foo> or <!-- > or <? > or <foo/> node.
		if (elementName == "?")
			state.parser.OnProcessingInstruction();
		else if (elementName == "!--")
			state.parser.OnComment("");
		else 
		{
			if (state.Cur() == '/')
			{
				
				
			state.parser.OnElementStart(elementName, 
										Attributes());
		}
		state.eat();
		return true;
	}

	if (elementName == "!--")
		return ParseComment(state);
	else if (elementName[0] == "?")
		return ParseProcessingInstruction(elementName);
	else
		return ParseElement(state, elementName);

}

static bool ParseElement(ParserState& state, const string& name)
{
	string::size_type nameBeginIdx = state.Idx();

	// parse attibutes
	Attributes attrs;
	while (true)
	{
		if (!state.SkipUntil("= \t>")) return false;

		string attrName = state.Slice(nameBeginIdx);
		
		if (!state.SkipWhitespace()) return false; 

		if (state.Cur() == '>')
		{
			// Node ended with attribute with no value
			attrs[attrName];
			break; // done
		}
		else if (state.Cur() != '=')
		{
			string msg = "Invalid character '";
			msg += state.Cur();
			msg += "' in element '";
			msg += name;
			msg += "' attributes";
			throw SaxParserException(state.line, msg);
		}

		if (!state.Skip()) return false;
		if (!state.SkipWhitespace()) return false;

		if (state.Cur() == '>')
		{
			// Node ended with attribute with no value
			attrs[attrName];
			break; // done
		}
		
		char quoteChar = state.Cur();
		if (quoteChar != '\'' && quoteChar != '"')
		{
			string msg = "Invalid quote character '";
			msg += state.Cur();
			msg += "' in element '";
			msg += name;
			msg += "' attributes";
			throw SaxParserException(state.line, msg);
		}
	}

	state.parse.OnElementStart(name, attrs);
}

	string::size_type endAttrIdx = data.find_first_of("=>", attribBeginIdx);
	Attributes attrs;
	while (endAttrIdx != npos) 
	{
		string attrName = data.substr(attrBeginIdx, endAttrIdx - attrBeginIdx);

		if (data[endAttrIdx] == '>')
		{
			attrs[attrName]; // no value
			parser.OnElementStart(elementName, attrs);
		}
		
		// must be '=' at attrEndIdx
		string::size_type endAttrValueIdx = data.find_first_of(" >", attribEndIdx);
		if (endAttrValueIdx == npos) return; // keep looking
		attribEndIdx++;

		if (data[endAttrValueIdx] == '>')
		{
			attrs[attrName] = data.substr(attribEndIdx, endAttrValueIdx - attribEndId);
			parser.OnElementStart(elementName, attrs);
		}
		
		
	}

	if (data[endAttrIdx] == '>')
	{
		parser.OnElementStart(data.substr(nameBeginIdx+1, nameEndIdx-nameBeginIdx-1), 
					   Attributes());
		attribBeginIdx++;
		if (attribBeginIdx >= len)
			data.clear();
		else
			data = data.substr(attribBeginIdx);
		
	}

	int nesting = data[endIdx] == '"' ? 1 : 0;
	while (nesting)
	{
		// Look for ending " taking \" into consideration
		
		endIdx = data.find_first_of("\">", startIdx);
		
	}
	*/
	
}

static void ParseProcessingInstruction(ParserState& state, const string& name)
{
}

static void ParseComment(ParserState& state)
{
}

}

void SaxParser::Feed(const std::string& data)
{
	m_Buffer += data;
	
	ParseNonText(*this, m_Buffer);
}


} // end namespace crudesax
