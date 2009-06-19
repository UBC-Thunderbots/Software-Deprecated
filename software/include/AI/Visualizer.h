#ifndef AI_VISUALIZER_H
#define AI_VISUALIZER_H

#include <sigc++/sigc++.h>
#include <gtkmm.h>

class Visualizer : public Gtk::DrawingArea, public virtual sigc::trackable {
public:
	Visualizer();
	void update(void);

private:
	Gtk::Window win;
	bool on_expose_event(GdkEventExpose *event);
	bool on_close(GdkEventAny *event);
};

#endif
