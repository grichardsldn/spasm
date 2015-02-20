extern struct text_file * text_init(char *filename);

extern int text_get_next_token(struct text_file *me, char *token, int length);
extern int text_get_lineno(struct text_file *me);

extern int text_reset_file(struct text_file *me);
extern int text_set_offset(struct text_file *me, int offset) ;
extern int text_get_current_line(struct text_file *me, char *buffer, int bufflen);
extern int text_move_to_next_line(struct text_file *me);
extern int text_set_line(struct text_file *me, int lineno);
