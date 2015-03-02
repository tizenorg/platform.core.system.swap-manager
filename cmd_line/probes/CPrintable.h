#ifndef __CPRINTABLE_H__
#define __CPRINTABLE_H__

enum output_mode_t{
	OM_SIMPLE = 0,
	OM_DEBUG = 1
};

class CPrintable {
	public:
		virtual void printList() = 0;
		virtual void printElm() = 0;
};

#endif /* __CPRINTABLE_H__ */
