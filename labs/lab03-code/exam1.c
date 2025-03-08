typedef struct {
    char name[32];
    float gpa;
    unsigned int credits;
    } student_t;


int write_highest_credits(char *in_file, char *out_file) {
    FILE *f1 = fopen(in_file, "r");
    if( f1 == NULL) return -1;

    FILE *f2 = fopen(out_file, "w");
    if(f2 == NULL){
        fclose(f1);
        return -1;
    }
    student_t top_student;
    student_t student;

    while (fread(&student, sizeof(student_t), 1, f1) == 1){
        if (student.credits > top_student.credits){
            top_student = student;
        }
        fseek(f1, 100, SEEK_CUR);
    }
    fclose(f1);
    fwrite(&top_student, sizeof(student_t), 1, f2);
    fclose(f2);
    return 0;
}