/* hospital_by_name.c
   Minimal hospital appointment demo.
   - Patients: linked list, auto-increment id
   - Doctors: BST, auto-increment id
   - Requests: circular queue (string date/time)
   - Scheduling: array of schedules, undo via stack (stores schedule id)
   - show scheduled appointments prints patient & doctor NAMES
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_LEN 48
#define TIME_LEN 64
#define QCAP 8
#define SCHED_CAP 100
#define STCAP 50

/* Types */
typedef struct Req
{
    int id, pid, did;
    char t[TIME_LEN];
} Req;
typedef struct D
{
    int id;
    char name[NAME_LEN];
    struct D *l, *r;
} D;
typedef struct P
{
    int id;
    char name[NAME_LEN];
    struct P *next;
} P;
typedef struct S
{
    int sid, pid, did;
    char t[TIME_LEN];
    int active;
} Sched;

/* Globals */
static Req q[QCAP];
static int qf = -1, qr = -1, qnext = 1;

static D *droot = NULL;
static int dnext = 1;

static P *phead = NULL;
static int pnext = 1;

static Sched sched[SCHED_CAP];
static int scount = 0, snext = 1;

static int st[STCAP];
static int stp = -1;

/* Helpers */
static void scpy(char *d, const char *s, int n)
{
    if (!s)
    {
        d[0]=0;
        return;
    }
    strncpy(d,s,n-1);
    d[n-1]=0;
}

/* stack */
void st_push(int x)
{
    if (stp >= STCAP-1)
    {
        printf("undo stack full\n");
        return;
    }
    st[++stp]=x;
}
int st_pop()
{
    if (stp < 0) return -1;
    return st[stp--];
}


/* queue */
int q_empty()
{
    return qf == -1;
}
int q_full()
{
    return qf == (qr+1)%QCAP;
}

void q_enq(int pid, int did, const char *time)
{
    if (q_full())
    {
        printf("Request queue is full\n");
        return;
    }
    if (qf == -1) qf = qr = 0;
    else qr = (qr+1)%QCAP;
    q[qr].id = qnext++;
    q[qr].pid = pid;
    q[qr].did = did;
    scpy(q[qr].t, time, TIME_LEN);
    printf("Enqueued request #%d  patient %d -> doctor %d  at %s\n", q[qr].id, pid, did, q[qr].t);
}

Req q_deq()
{
    Req e = {-1,-1,-1,""};
    if (q_empty()) return e;
    Req r = q[qf];
    if (qf == qr) qf = qr = -1;
    else qf = (qf+1)%QCAP;
    return r;
}

void q_show()
{
    if (q_empty())
    {
        printf("(request queue empty)\n");
        return;
    }
    printf("Request queue :\n");
    int i=qf;
    while(1)
    {
        printf("  #%d  p%d -> d%d  @ %s\n", q[i].id, q[i].pid, q[i].did, q[i].t);
        if (i==qr) break;
        i=(i+1)%QCAP;
    }
}

/* doctors BST */
D* d_new(int id, const char *name)
{
    D *n=malloc(sizeof(D));
    if(!n)
    {
        perror("malloc");
        exit(1);
    }
    n->id=id;
    scpy(n->name,name,NAME_LEN);
    n->l=n->r=NULL;
    return n;
}
D* bst_ins(D *r, int id, const char *name)
{
    if (!r) return d_new(id,name);
    if (id <= r->id) r->l = bst_ins(r->l,id,name);
    else r->r = bst_ins(r->r,id,name);
    return r;
}
D* bst_find(D *r, int id)
{
    if (!r) return NULL;
    if (r->id==id) return r;
    if (id < r->id) return bst_find(r->l,id);
    return bst_find(r->r,id);
}
void bst_in(D *r)
{
    if (!r) return;
    bst_in(r->l);
    printf("  %d : %s\n", r->id, r->name);
    bst_in(r->r);
}
void bst_free(D *r)
{
    if (!r) return;
    bst_free(r->l);
    bst_free(r->r);
    free(r);
}

/* patients (singly linked list) */
P* p_find(int id)
{
    P *c=phead;
    while(c)
    {
        if (c->id==id) return c;
        c=c->next;
    }
    return NULL;
}
void p_add(const char *name)
{
    P *n=malloc(sizeof(P));
    if(!n)
    {
        perror("malloc");
        exit(1);
    }
    n->id=pnext++;
    scpy(n->name,name,NAME_LEN);
    n->next=phead;
    phead=n;
    printf("Added patient %d : %s\n", n->id, n->name);
}
void p_del(int id)
{
    P *c=phead,*pr=NULL;
    while(c)
    {
        if(c->id==id)
        {
            if(pr) pr->next=c->next;
            else phead=c->next;
            free(c);
            printf("Deleted patient %d\n", id);
            return;
        }
        pr=c;
        c=c->next;
    }
    printf("Patient %d not found\n", id);
}
void p_list()
{
    if(!phead)
    {
        printf("(no patients)\n");
        return;
    }
    P*c=phead;
    printf("Patients (id : name):\n");
    while(c)
    {
        printf("  %d : %s\n", c->id, c->name);
        c=c->next;
    }
}

/* scheduling */
void process_one()
{
    Req r = q_deq();
    if (r.id == -1)
    {
        printf("No requests to process\n");
        return;
    }
    P *pp = p_find(r.pid);
    D *dd = bst_find(droot, r.did);
    if (!pp)
    {
        printf("Patient %d not found — discarding request\n", r.pid);
        return;
    }
    if (!dd)
    {
        printf("Doctor %d not found — discarding request\n", r.did);
        return;
    }
    if (scount >= SCHED_CAP)
    {
        printf("Scheduled storage full\n");
        return;
    }
    int sid = snext++;
    sched[scount].sid = sid;
    sched[scount].pid = r.pid;
    sched[scount].did = r.did;
    scpy(sched[scount].t, r.t, TIME_LEN);
    sched[scount].active = 1;
    scount++;
    st_push(sid);
    printf("Scheduled appointment s%d: patient %s (p%d) -> doctor %s (d%d) at %s (from req #%d)\n",
           sid, pp->name, pp->id, dd->name, dd->id, r.t, r.id);
}

void undo_one()
{
    int sid = st_pop();
    if (sid == -1)
    {
        printf("Nothing to undo\n");
        return;
    }
    for (int i=0; i<scount; i++)
    {
        if (sched[i].sid == sid)
        {
            if (!sched[i].active)
            {
                printf("Appointment s%d already undone\n", sid);
                return;
            }
            sched[i].active = 0;
            P *pp = p_find(sched[i].pid);
            D *dd = bst_find(droot, sched[i].did);
            printf("Undid appointment s%d (patient %s p%d doctor %s d%d at %s)\n",
                   sid,
                   pp ? pp->name : "[deleted]", sched[i].pid,
                   dd ? dd->name : "[deleted]", sched[i].did,
                   sched[i].t);
            return;
        }
    }
    printf("Scheduled id %d not found\n", sid);
}

void show_sched()
{
    if (scount==0)
    {
        printf("(no scheduled appointments)\n");
        return;
    }
    printf("Scheduled appointments:\n");
    for(int i=0; i<scount; i++)
    {
        P *pp = p_find(sched[i].pid);
        D *dd = bst_find(droot, sched[i].did);
        printf("  s%d: %s (p%d) -> %s (d%d) at %s [%s]\n",
               sched[i].sid,
               pp ? pp->name : "[deleted]", sched[i].pid,
               dd ? dd->name : "[deleted]", sched[i].did,
               sched[i].t,
               sched[i].active ? "active":"undone");
    }
}

/* add doctor with auto id */
void add_doctor_auto(const char *name)
{
    int id = dnext++;
    droot = bst_ins(droot, id, name);
    printf("Added doctor %d : %s\n", id, name);
}

void d_list()
{
    if (!droot)
    {
        printf("(no doctors)\n");
        return;
    }
    printf("Doctors (id : name):\n");
    bst_in(droot);
}

/* Menu & I/O helpers */
void menu()
{
    puts("\n=== Hospital Simple Menu ===");
    puts("1  - Add patient");
    puts("2  - Delete patient");
    puts("3  - List patients");
    puts("4  - Add doctor ");
    puts("5  - List doctors");
    puts("6  - Enqueue appointment request");
    puts("7  - Show request queue");
    puts("8  - Process next request (schedule)");
    puts("9  - Undo last scheduled appointment");
    puts("10 - Show scheduled appointments ");
    puts("0  - Exit");
}

static void read_line(char *buf, int n)
{
    if (!fgets(buf, n, stdin))
    {
        buf[0]=0;
        return;
    }
    buf[strcspn(buf,"\n")] = 0;
}

int main(void)
{
    for(;;)
    {
        menu();
        printf("Choose an option: ");
        int c;
        if (scanf("%d",&c)!=1) break;
        while(getchar()!='\n'); /* clear rest of line */

        if (c==0) break;

        if (c==1)
        {
            char s[NAME_LEN];
            printf("Enter patient name: ");
            read_line(s, sizeof(s));
            if (strlen(s)) p_add(s);
        }
        else if (c==2)
        {
            int id;
            printf("Enter patient id to delete: ");
            if(scanf("%d",&id)==1)
            {
                while(getchar()!='\n');
                p_del(id);
            }
            else
            {
                while(getchar()!='\n');
            }
        }
        else if (c==3) p_list();
        else if (c==4)
        {
            char s[NAME_LEN];
            printf("Enter doctor name: ");
            read_line(s, sizeof(s));
            if (strlen(s)) add_doctor_auto(s);
        }
        else if (c==5) d_list();
        else if (c==6)
        {
            int pid,did;
            char t[TIME_LEN];
            printf("Enter patient id: ");
            if(scanf("%d",&pid)!=1)
            {
                while(getchar()!='\n');
                continue;
            }
            printf("Enter doctor id: ");
            if(scanf("%d",&did)!=1)
            {
                while(getchar()!='\n');
                continue;
            }
            while(getchar()!='\n');
            printf("Enter date/time (string): ");
            read_line(t, sizeof(t));
            q_enq(pid,did,t);
        }
        else if (c==7) q_show();
        else if (c==8) process_one();
        else if (c==9) undo_one();
        else if (c==10) show_sched();
        else printf("Unknown option\n");
    }

    /* cleanup */
    bst_free(droot);
    while(phead)
    {
        P *t=phead;
        phead=phead->next;
        free(t);
    }
    printf("Goodbye\n");
    return 0;
}

