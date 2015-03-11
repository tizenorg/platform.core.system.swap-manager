#ifndef __CFEATURES_H__
#define __CFEATURES_H__

#include <bitset>
#include <string>
#include "debug.h"

#define FEATURE_MAX_SIZE 128
#define FEATURE_CELL_SIZE 64

extern const char *feature_to_str_arr[];

class CFeatures : public std::bitset<FEATURE_MAX_SIZE>, public CPrintable
{
	protected:
		typedef std::bitset<FEATURE_CELL_SIZE> CBitSet64;

		std::string featureToStr()
		{
			int i;
			std::string res;

			for (i = 0; i < FEATURE_MAX_SIZE; i++) {
				if (CFeatures::test(i)) {
					std::string tmp(feature_to_str_arr[i]);
					/* TODO make a constant or define */
					res = "setv feature enable " + tmp + "\n" + res;
				}
			}

		exit:
			return res;
		}

		int serialize(uint64_t *f, int count64bit)
		{
			int i, pos, res;
			std::string st;
			CBitSet64 bitset64;

			res = 0;

			st = to_string();
			pos = st.length();

			for (i = 0; i < count64bit; i++) {
				pos -= FEATURE_CELL_SIZE;
				if (pos < 0) {
					res = -EINVAL;
					goto exit;
				}

				bitset64 = CBitSet64(st.substr(pos, FEATURE_CELL_SIZE));
				TRACE("%s", bitset64.to_string().c_str());

				f[i] = bitset64.to_ullong();
				TRACE("%d:%lx", i, f[i]);
			}

			exit:
			return res;
		}

	public:

		virtual void printElm()
		{
			uint64_t features[FEATURE_MAX_SIZE/FEATURE_CELL_SIZE];
			std::string st = to_string();

			TRACE("%s", st.c_str());

			if (serialize(features, sizeof(features)/sizeof(*features)) == 0) {
				st = featureToStr();
				TRACE("\n%s", st.c_str());
			}
		}

		virtual void printList()
		{
			printElm();
		}

		CFeatures()
		{
			TRACE("Create");
		}

		virtual ~CFeatures()
		{
			TRACE("Destroy");
		}
};
#endif /* __CFEATURES_H__ */
