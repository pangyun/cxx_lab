%{
    extern int yylex();
    void yyerror(const char *s) {std::printf("Error: %s\n", s); std::exit(1);}
%}
