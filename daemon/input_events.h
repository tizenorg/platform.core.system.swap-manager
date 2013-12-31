#define MAX_FILENAME 128

int init_input_events(void);
void add_input_events(void);
void del_input_events(void);
void write_input_event(int id, struct input_event *ev);
