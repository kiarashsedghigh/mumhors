#include "sort.h"

static void merge(int *arr, int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
    int L[n1], R[n2];

    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    i = 0;
    j = 0;
    k = l;
    while (i < n1 && j < n2) {
        if (L[i] >= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

static void merge_sort_desc(int * arr, int l, int r){
    if (l < r) {
        int m = l + (r - l) / 2;

        merge_sort_desc(arr, l, m);
        merge_sort_desc(arr, m + 1, r);

        merge(arr, l, m, r);
    }
}


static void insertion_sort_desc(int* arr, const int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;

        while (j >= 0 && arr[j] < key) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }

        arr[j + 1] = key;
    }
}

void array_sort(int* arr, const int n) {
    // insertion_sort_desc(arr, n);
    merge_sort_desc(arr, 0, n - 1);
}

