#ifndef __IINTERFACE_H__
#define __IINTERFACE_H__

class IInterface
{
public:
	IInterface() { };
	virtual ~IInterface() { };

	virtual int Initialize() = 0;
	virtual int Uninitialize() = 0;

private:
	IInterface(const IInterface& iSrc);              // no implementation
	void operator=(const IInterface& iSrc);       // no implementation
};

#endif // __IINTERFACE_H__
