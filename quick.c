#define POW2(E) 2 << (E - 1)

static bool first_run = true;
static Header* quick_fit_lists[NRQUICKLISTS];
static int smallest_block_size_exp = 5;

/**
 * Calculates the quick fit list index for the given number of bytes of data to
 * allocate (adding the size of the header in the calculations).
 */
int get_quick_fit_list_index(unsigned int nbytes) {
    unsigned int upper_bound;
    int i;
    for (i = 0; i < NRQUICKLISTS; ++i) {
        upper_bound = POW2(i + smallest_block_size_exp);
        if (nbytes + sizeof(Header) <= upper_bound) {
            return i;
        }
    }
    return NRQUICKLISTS;
}

/**
 * Gets more system from memory and initiates blocks of correct size.
 */
Header* init_quick_fit_list(int list_index) {
    int i;

    int block_size = POW2(list_index + smallest_block_size_exp); /* bytes, incl Header */
    int nbytes = sysconf(_SC_PAGESIZE);
    if (nbytes < 0) {
        return NULL;
    }
    /* Make sure at least one block fits */
    nbytes += (block_size / nbytes) * nbytes;
    int num_blocks = nbytes / block_size;

    char *cp;
    Header *up;
    cp = sbrk(nbytes);
    if (cp == (char *) -1) /* no space at all */
        return NULL;
    up = (Header *) cp;

    int nunits = block_size / sizeof(Header) - 1; /* excl Header */

    /* fixa blocken */
    up->s.ptr = up + 1 + nunits;
    up->s.size = nunits;

    for (i = 1; i < num_blocks; ++i) {
        up = up->s.ptr;
        up->s.ptr = up + 1 + nunits;
        up->s.size = nunits;
    }

    up->s.ptr = NULL;

    return (Header *) cp;
}

void *malloc_quick(size_t nbytes)
{
    Header *p, *prevp;
    Header *moreroce(unsigned);
    unsigned nunits;
    int list_index, i;

    nunits = (nbytes+sizeof(Header)-1)/sizeof(Header) + 1;
    list_index = get_quick_fit_list_index(nbytes);

    /* Use another strategy for too large allocations. */
    if (list_index >= NRQUICKLISTS) {
        return malloc_first(nbytes);
    }

    if (first_run) {
        /* Initialize the quick fit lists on first run. */
        for (i = 0; i < NRQUICKLISTS; ++i) {
            quick_fit_lists[i] = NULL;
        }
        first_run = false;
    }


    /* kolla om quick fit-listan redan är initierad OCH det finns ett ledigt block */
    if (quick_fit_lists[list_index] == NULL) {
        /*
         * - fråga systemet om lämpligt mkt mer minne
         * - bygger direkt en lista av fria block (i vårt exempel 64 bytes)
         *   som länkas in i fri-listan
         */
        Header* new_quick_fit_list = init_quick_fit_list(list_index);
        if (new_quick_fit_list == NULL) {
            return NULL;
        } else {
            quick_fit_lists[list_index] = new_quick_fit_list;
        }
    }

    void* pointer_to_return = (void *)(quick_fit_lists[list_index] + 1);
    quick_fit_lists[list_index] = quick_fit_lists[list_index]->s.ptr;
    return pointer_to_return;
}
