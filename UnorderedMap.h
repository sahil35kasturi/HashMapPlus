#include <cstddef>    // size_t
#include <functional> // std::hash
#include <ios>
#include <utility> // std::pair
#include <iostream>

#include "primes.h"

template <typename Key, typename T, typename Hash = std::hash<Key>, typename Pred = std::equal_to<Key>>
class UnorderedMap
{
public:
    using key_type = Key;
    using mapped_type = T;
    using const_mapped_type = const T;
    using hasher = Hash;
    using key_equal = Pred;
    using value_type = std::pair<const key_type, mapped_type>;
    using reference = value_type &;
    using const_reference = const value_type &;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

private:
    struct HashNode
    {
        HashNode *next;
        value_type val;

        HashNode(HashNode *next = nullptr) : next(next) {}
        HashNode(const value_type &val, HashNode *next = nullptr) : next(next), val(val) {}
        HashNode(value_type &&val, HashNode *next = nullptr) : next(next), val(std::move(val)) {}
    };

    size_type _bucket_count;
    HashNode **_buckets;

    HashNode *_head;
    size_type _size;

    Hash _hash;
    key_equal _equal;

    static size_type _range_hash(size_type hash_code, size_type bucket_count)
    {
        return hash_code % bucket_count;
    }

public:
    template <typename pointer_type, typename reference_type, typename _value_type>
    class basic_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = _value_type;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

    private:
        friend class UnorderedMap<Key, T, Hash, key_equal>;
        using HashNode = typename UnorderedMap<Key, T, Hash, key_equal>::HashNode;

        const UnorderedMap *_map;
        HashNode *_ptr;

        explicit basic_iterator(UnorderedMap const *map, HashNode *ptr) noexcept : _map{map}, _ptr{ptr} {}

    public:
        basic_iterator() : _map(nullptr), _ptr(nullptr){};

        basic_iterator(const basic_iterator &) = default;
        basic_iterator(basic_iterator &&) = default;
        ~basic_iterator() = default;
        basic_iterator &operator=(const basic_iterator &) = default;
        basic_iterator &operator=(basic_iterator &&) = default;
        reference operator*() const
        {
            return _ptr->val;
        }
        pointer operator->() const
        {
            return &(_ptr->val);
        }
        basic_iterator &operator++()
        {
            if (_ptr)
            {
                if (_ptr->next)
                {
                    _ptr = _ptr->next;
                }
                else
                {
                    size_type currentBucketIndex = _map->_range_hash(_map->_hash(_ptr->val.first), _map->_bucket_count);
                    size_type nextBucketIndex = currentBucketIndex + 1;
                    while (nextBucketIndex < _map->_bucket_count && !_map->_buckets[nextBucketIndex])
                        ++nextBucketIndex;

                    if (nextBucketIndex < _map->_bucket_count)
                    {
                        _ptr = _map->_buckets[nextBucketIndex];
                    }
                    else
                    {
                        _ptr = nullptr;
                    }
                }
            }
            return *this;
        }
        basic_iterator operator++(int)
        {
            basic_iterator temp = *this;
            ++(*this);
            return temp;
        }
        bool operator==(const basic_iterator &other) const noexcept
        {
            return (_ptr == other._ptr);
        }
        bool operator!=(const basic_iterator &other) const noexcept
        {
            return (_ptr != other._ptr);
        }
    };

    using iterator = basic_iterator<pointer, reference, value_type>;
    using const_iterator = basic_iterator<const_pointer, const_reference, const value_type>;

    class local_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::pair<const key_type, mapped_type>;
        using difference_type = ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;

    private:
        friend class UnorderedMap<Key, T, Hash, key_equal>;
        using HashNode = typename UnorderedMap<Key, T, Hash, key_equal>::HashNode;

        HashNode *_node;

        explicit local_iterator(HashNode *node) noexcept : _node{node} {}

    public:
        local_iterator() : _node(nullptr) {}

        local_iterator(const local_iterator &) = default;
        local_iterator(local_iterator &&) = default;
        ~local_iterator() = default;
        local_iterator &operator=(const local_iterator &) = default;
        local_iterator &operator=(local_iterator &&) = default;
        reference operator*() const
        {
            return this->_node->val;
        }
        pointer operator->() const
        {
            return &this->_node->val;
        }
        local_iterator &operator++()
        {
            _node = _node->next;
            return *this;
        }
        local_iterator operator++(int)
        {
            local_iterator temp = *this;
            ++(*this);
            return temp;
        }

        bool operator==(const local_iterator &other) const noexcept
        {
            return (this->_node == other._node);
        }
        bool operator!=(const local_iterator &other) const noexcept
        {
            return (this->_node != other._node);
        }
    };

private:
    size_type _bucket(size_t code) const
    {
        return _range_hash(code, this->_bucket_count);
    }
    size_type _bucket(const Key &key) const
    {
        return _bucket(_hash(key));
    }
    size_type _bucket(const value_type &val) const
    {
        return _bucket(val.first);
    }

    HashNode *&_find(size_type code, size_type bucket, const Key &key)
    {
        HashNode **curr = &_buckets[bucket];
        while (*curr != nullptr)
        {
            if (_equal((*curr)->val.first, key))
            {
                return *curr;
            }
            curr = &((*curr)->next);
        }
        return *curr;
    }

    HashNode *&_find(const Key &key)
    {
        size_type code = _hash(key);
        size_type bucket = _bucket(code);
        return _find(code, bucket, key);
    }
    // fix this
    HashNode *_insert_into_bucket(size_type bucket, value_type &&value)
    {
        HashNode *newNode = new HashNode(std::move(value));
        newNode->next = _buckets[bucket];
        _buckets[bucket] = newNode;

        if (!_head || bucket <= _bucket(_head->val.first))
        {
            _head = newNode;
        }

        return newNode;
    }

    void _move_content(UnorderedMap &src, UnorderedMap &dst)
    {
        dst._size = src._size;
        dst._hash = src._hash;
        dst._equal = src._equal;
        dst._bucket_count = src._bucket_count;
        dst._buckets = src._buckets;
        dst._head = src._head;

        src._size = 0;
        src._buckets = new HashNode *[src._bucket_count]();
        src._head = nullptr;
    }

public:
    explicit UnorderedMap(size_type bucket_count, const Hash &hash = Hash{},
                          const key_equal &equal = key_equal{})
    {
        _bucket_count = next_greater_prime(bucket_count);

        _buckets = new HashNode *[_bucket_count] {};

        _head = nullptr;
        _size = 0;
        _hash = hash;
        _equal = equal;
    }

    ~UnorderedMap()
    {
        clear();
        delete[] _buckets;
    }
    // copy constructor seg fault
    UnorderedMap(const UnorderedMap &other)
    {
        _bucket_count = other._bucket_count;
        _size = 0;

        _buckets = new HashNode *[_bucket_count] {};

        _hash = other._hash;
        _equal = other._equal;
        _head = nullptr;
        for (size_type i = 0; i < _bucket_count; ++i)
        {
            for (HashNode *node = other._buckets[i]; node; node = node->next)
            {
                insert(node->val);
            }
        }
    }

    UnorderedMap(UnorderedMap &&other)
    {
        _move_content(other, *this);
    }

    UnorderedMap &operator=(const UnorderedMap &other)
    {
        if (this != &other)
        {
            clear();
            delete[] _buckets;
            _bucket_count = other._bucket_count;
            _size = 0;

            _buckets = new HashNode *[_bucket_count] {};

            _hash = other._hash;
            _equal = other._equal;

            for (size_type i = 0; i < _bucket_count; ++i)
            {
                for (HashNode *node = other._buckets[i]; node; node = node->next)
                {
                    insert(node->val);
                }
            }
        }
        return *this;
    }

    UnorderedMap &operator=(UnorderedMap &&other)
    {
        if (this != &other)
        {
            clear();
            delete[] _buckets;
            _move_content(other, *this);
        }
        return *this;
    }

    void clear() noexcept
    {
        while (_size != 0)
        {
            erase(begin());
        }
    }

    size_type size() const noexcept
    {
        return this->_size;
    }

    bool empty() const noexcept
    {
        return this->_size == 0;
    }

    size_type bucket_count() const noexcept
    {
        return this->_bucket_count;
    }

    iterator begin() noexcept
    {
        return iterator(this, _head);
    }
    iterator end() noexcept
    {
        return iterator(this, nullptr);
    }

    const_iterator cbegin() const
    {
        return const_iterator(this, _head);
    }
    const_iterator cend() const
    {
        return const_iterator(this, nullptr);
    }

    local_iterator begin(size_type n)
    {
        return local_iterator(_buckets[n]);
    }
    local_iterator end(size_type n)
    {
        return local_iterator(nullptr);
    }

    size_type bucket_size(size_type n)
    {
        local_iterator temp = begin(n);
        size_type size = 0;
        while (temp != end(n))
        {
            ++temp;
            ++size;
        }
        return size;
    }

    float load_factor() const
    {
        return static_cast<float>(size()) / bucket_count();
    }

    size_type bucket(const Key &key) const
    {
        return _hash(key) % _bucket_count;
    }

    std::pair<iterator, bool> insert(value_type &&value)
    {
        size_type index = _bucket(value.first);
        HashNode *node = _find(_hash(value.first), index, value.first);

        if (node == nullptr)
        {
            node = _insert_into_bucket(index, std::move(value));
            ++_size;
            return {iterator(this, node), true};
        }
        else
        {
            return {iterator(this, node), false};
        }
    }

    std::pair<iterator, bool> insert(const value_type &value)
    {
        size_type index = _bucket(value.first);

        HashNode *current = _buckets[index];
        while (current != nullptr)
        {
            if (_equal(current->val.first, value.first))
            {
                return std::make_pair(iterator(this, current), false);
            }
            current = current->next;
        }
        // fix this
        value_type newval = value;
        HashNode *newNode = _insert_into_bucket(index, std::move(newval));
        ++_size;

        return std::make_pair(iterator(this, newNode), true);
    }

    iterator find(const Key &key)
    {
        return iterator(this, _find(key));
    }

    T &operator[](const Key &key)
    {
        value_type val = std::make_pair(key, mapped_type());
        T &result = (insert(val).first._ptr->val.second);
        return result;
    }

    iterator erase(iterator pos)
    {
        if (pos == end())
        {
            return end();
        }

        HashNode *&m = _find(pos->first);

        if (m == nullptr)
        {
            return end();
        }

        HashNode *del = m;
        ++pos;

        if (_head == del)
        {
            _head = pos._ptr;
        }

        m = m->next;
        delete del;
        _size--;

        return pos;
    }

    size_type erase(const Key &key)
    {
        HashNode *&node = _find(key);

        if (node != nullptr)
        {
            HashNode *toDelete = node;

            if (_head == node)
            {
                iterator i(this, node);
                i++;
                _head = i._ptr;
            }
            node = node->next;

            delete toDelete;
            _size--;
            return 1;
        }
        else
        {
            return 0;
        }
    }

    template <typename KK, typename VV>
    friend void print_map(const UnorderedMap<KK, VV> &map, std::ostream &os);
};

template <typename K, typename V>
void print_map(const UnorderedMap<K, V> &map, std::ostream &os = std::cout)
{
    using size_type = typename UnorderedMap<K, V>::size_type;
    using HashNode = typename UnorderedMap<K, V>::HashNode;

    for (size_type bucket = 0; bucket < map.bucket_count(); bucket++)
    {
        os << bucket << ": ";

        HashNode const *node = map._buckets[bucket];

        while (node)
        {
            os << "(" << node->val.first << ", " << node->val.second << ") ";
            node = node->next;
        }

        os << std::endl;
    }
}
