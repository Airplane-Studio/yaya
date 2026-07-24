#pragma once

enum ErrorLevel {
    ERROR,
    WARNING,
    NOTE
};

class ErrorReport {
private:
    DynamicArray<Token> orig;

    UTF8String err2str(ErrorLevel level) {
        switch (level) {
            case ERROR: return "\x1b[31merror";
            case WARNING: return "\x1b[35mwarning";
            case NOTE: return "\x1b[36mnote";
            default: return "";
        }
    }
public:
    ErrorReport(DynamicArray<Token> &orig) : orig(orig) {}

    void log(ErrorLevel level, int report_idx, UTF8String msg, bool resolve_pos = true) {
        Token tok;
        DynamicArray<Token> line;
        if (report_idx != -1) {
            tok = orig[report_idx];
            int cur_idx = report_idx;
            while (cur_idx) {
                if (orig[cur_idx].line != tok.line) break;
                cur_idx--;
            }
            if (cur_idx || orig[cur_idx].type == TT_NEWLINE) cur_idx++;
            int off = report_idx - cur_idx;
            while (cur_idx < orig.size() && orig[cur_idx].type != TT_NEWLINE) {
                line.append(orig[cur_idx]);
                cur_idx++;
            }
            if (cur_idx < orig.size()) line.append(orig[cur_idx]);
            if (resolve_pos) {
                for (int i = 1; i < line.size(); i++) {
                    line[i].start_col += line[i - 1].end_col;
                    line[i].end_col += line[i - 1].end_col;
                }
            }
            //for (int i = 0; i < line.size(); i++) line[i].line = lineno;
            tok = line[off];
        } else line.append(tok);
        io.print("\x1b[1m", tok.src_file, ":", tok.line, ":", tok.start_col, ": ");
        io.println(err2str(level), ":\x1b[39;49;0m ", msg);
        TokenPrettifier::print_tokens(line);
        for (int i = 0; i < TokenPrettifier::num_len(tok.line) + 3; i++) io.print(' ');
        for (int i = 0; i < tok.start_col - 1; i++) io.print(' ');
        io.print("^");
        for (int i = tok.start_col + 1; i <= tok.end_col; i++) io.print("~");
        io.println();
    }
};