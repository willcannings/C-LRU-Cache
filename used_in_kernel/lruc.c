#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hashtable.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("yang.yang <yang.yang2@cloudscreen.com>");
MODULE_DESCRIPTION("In-kernel reverse dns map");

static uint32_t cache_size = 1024 * 1024;
module_param(cache_size, uint, (S_IRUSR | S_IRGRP | S_IROTH));
MODULE_PARM_DESC(cache_size, "cache buffer size");

static LIST_HEAD(lru_list);
static uint32_t free_memory, seed;
//create hash table includes hash_table_size slots.
DEFINE_HASHTABLE(htable, 12);
DEFINE_MUTEX(cache_lock);

struct lru_node {
    void *hash_table_node;
    struct list_head list;
};

//hash table object
struct hash_node {
    void *key;
    void *value;

    uint32_t key_length;
    uint32_t value_length;

    struct lru_node *list_node;

    struct hlist_node node;
};

static void lruc_new(void) {
    struct timespec ts;
    getnstimeofday(&ts);
    seed = ts.tv_sec;
    free_memory = cache_size;
}

static void lruc_free(void) {
    struct hash_node *obj;
    uint32_t bkt;
    hash_for_each(htable, bkt, obj, node) {
        kfree(obj->list_node);
        kfree(obj->key);
        kfree(obj->value);
        kfree(obj);
    }
}

static int __init lru_init(void) {
    if (!cache_size) {
        return -1;
    }

    lruc_new();

    printk(KERN_INFO "Hello, lru\n");
    return 0;
}

static void __exit lru_exit(void) {
    lruc_free();
    printk(KERN_INFO "Goodbye, lru\n");
}

static uint32_t lruc_hash(void *key, uint32_t key_length) {
    uint32_t m = 0x5bd1e995;
    uint32_t r = 24;
    uint32_t h = seed ^ key_length;
    char *data = (char *)key;

    while (key_length >= 4) {
        uint32_t k = *(uint32_t *)data;
        k *= m;
        k ^= k >> r;
        k *= m;
        h *= m;
        h ^= k;
        data += 4;
        key_length -= 4;
    }

    switch (key_length) {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
    };

    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;
    return h;
}

void lruc_get(void *key, uint32_t key_length, void **value) {
    uint32_t id = lruc_hash(key, key_length);

    struct hash_node *obj;
    mutex_lock(&cache_lock);
    hash_for_each_possible(htable, obj, node, id) {
        //key exist already
        if (obj->key_length == key_length && !memcmp(obj->key, key, key_length)) {
            //reference the double list node
            *value = obj->value;
            list_move_tail(&obj->list_node->list, &lru_list);
            mutex_unlock(&cache_lock);
            return;
        }
    }
    mutex_unlock(&cache_lock);

    *value = NULL;
}

void lruc_set(void *key, uint32_t key_length, void *value, uint32_t value_length) {
    int exist = 0;
    uint32_t id = lruc_hash(key, key_length);

    uint32_t required = 0;
    struct hash_node *obj;
    struct lru_node *lru_obj;
    void *k, *v;

    mutex_lock(&cache_lock);
    hash_for_each_possible(htable, obj, node, id) {
        //key exist already
        if (obj->key_length == key_length && !memcmp(obj->key, key, key_length)) {
            kfree(obj->value);
            v = kmalloc(value_length, GFP_KERNEL);
            memcpy(v, value, value_length);
            obj->value = v;
            if (value_length > obj->value_length) {
                required = value_length - obj->value_length;
            }
            obj->value_length = value_length;

            //reference the double list node
            list_move_tail(&obj->list_node->list, &lru_list);
            exist = 1;
            break;
        }
    }

    if (!exist) {
        required = key_length + value_length;

        obj = (struct hash_node*) kmalloc(sizeof(struct hash_node), GFP_KERNEL);
        k = kmalloc(key_length, GFP_KERNEL);
        memcpy(k, key, key_length);
        obj->key = k;
        obj->key_length = key_length;
        v = kmalloc(value_length, GFP_KERNEL);
        memcpy(v, value, value_length);
        obj->value = v;
        obj->value_length = value_length;
        obj->list_node = (struct lru_node*) kmalloc(sizeof(struct lru_node), GFP_KERNEL);
        obj->list_node->hash_table_node = obj;

        list_add_tail(&obj->list_node->list, &lru_list);
        hash_add(htable, &obj->node, id);
    }

    while (required > 0 && required > free_memory) {
        //the first node from list
        struct list_head *node = (&lru_list)->next;
        lru_obj = list_entry(node, struct lru_node, list);
        if (lru_obj != NULL) {
            obj = lru_obj->hash_table_node;
            if (obj != NULL) {
                kfree(obj->key);
                kfree(obj->value);
                list_del_init(node);
                free_memory += obj->key_length;
                free_memory += obj->value_length;
                hash_del(&obj->node);

                kfree(lru_obj);
                kfree(obj);
            }
        }
    }

    mutex_unlock(&cache_lock);

    free_memory -= required;
}

EXPORT_SYMBOL(lruc_set);
EXPORT_SYMBOL(lruc_get);
module_init(lru_init);
module_exit(lru_exit);
