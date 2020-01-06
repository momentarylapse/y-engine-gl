/*
 * Text.h
 *
 *  Created on: 04.01.2020
 *      Author: michi
 */

#ifndef SRC_GUI_TEXT_H_
#define SRC_GUI_TEXT_H_

#include "Picture.h"

class Text : public Picture {
public:
	Text(const string &t, const vector &p, float h);
	virtual ~Text();

	void rebuild() override;

	string text;
};

#endif /* SRC_GUI_TEXT_H_ */