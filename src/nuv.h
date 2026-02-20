typedef struct {
        int vote,
            counts,
            sl;

        char say[41],
             name[41];
} vcrec;

typedef struct {
        int num,
            age;
        char firston[8],
            name[41],
             snote[MAX_PATH_LEN];
        int vote_yes,
            vote_no,
            vcmt_num;
         vcrec vote_comment[20];
} nuvdata;
