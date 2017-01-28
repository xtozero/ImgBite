#pragma once

class JFIF;

class JFIFDebugger
{
public:
	void PrintQuantTable() const;
	void PrintHuffmanTable( ) const;
	
	JFIFDebugger( JFIF& jfif ) noexcept : m_debugTarget( jfif ) {}

private:
	JFIF& m_debugTarget;
};
