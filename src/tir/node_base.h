#pragma once

#include <optional>
#include <string>

namespace tdc {
namespace tir {

enum class NodeType {
    // expr node
    // stmt node
};

// a wrapper for std::is_base_of
// old versions of g++ have std::is_base_of<Cls, Cls> = false
// which is not expected
template <typename Base, typename Derived>
struct is_base_of_t {
    using RMConstDerived = typename std::remove_const<Derived>::type;
    static constexpr bool value =
        std::is_base_of<Base, RMConstDerived>::value ||
        std::is_same<Base, RMConstDerived>::value;
};

/**
 * The smart pointer for statement nodes, expression node and etc.
 * Uses std::shared_ptr as internal implementation. It reinforces
 * shared_ptr by adding upcast/downcast functionalities.
 *
 * @tparam T the type of the node that this NodePtr points to
 * @tparam Base the root base class of T. Examples: stmt_base_t, expr_base
 * */
template <typename T, typename Base>
class NodePtrImpl {
   public:
    template <typename _Arg>
    using _assignable =
        typename std::enable_if<std::is_convertible<_Arg*, T*>::value,
                                Base>::type;
    static_assert(is_base_of_t<Base, T>::value,
                  "T should be a subclass of Base");

    // the implementation is based on shared_ptr<Base>
    using impl_ptr = std::shared_ptr<Base>;
    using type = T;

    impl_ptr impl;

    // constructible from a sub-class NodePtr<T2, Base>, where T2 < T
    template <typename T2>
    NodePtrImpl(const NodePtr<T2, _assignable<T2>>& other) noexcept
        : impl(other.impl) {}

    // Move-constructible from a sub-class NodePtr<T2, Base>, where T2 < T
    template <typename T2>
    NodePtrImpl(NodePtr<T2, _assignable<T2>>&& other) noexcept
        : impl(std::move(other.impl)) {}

    // Constructs a NodePtrImpl from raw shared_ptr<Base>
    explicit NodePtrImpl(const impl_ptr& impl) noexcept : impl(impl) {}
    // Move-constructs a NodePtrImpl from raw shared_ptr<Base>
    explicit NodePtrImpl(impl_ptr&& impl) noexcept : impl(std::move(impl)) {}

    // Constructs an empty NodePtrImpl
    NodePtrImpl() noexcept = default;

    template <typename T2>
    NodePtrImpl& operator=(const NodePtr<T2, _assignable<T2>>& other) noexcept {
        impl = other.impl;
        return *this;
    }

    // move-assignable from NodePtr
    template <typename T2>
    NodePtrImpl& operator=(NodePtr<T2, _assignable<T2>>&& other) noexcept {
        impl = std::move(other.impl);
        return *this;
    }

    /**
     * Checks if the contained pointer is `exactly` of type T2::type.
     * `defined()` of this must be `true`. See notes.
     *
     * @tparam T2 the type to check. T2 should be NodePtr<X, Base> or its alias
     * like `for_loop`
     * @return `true` if the contained pointer is of type T2.
     * @note isa() will not check if the pointer is a sub-class of T2::type.
     *  That is, for example:
     *      add a = ...;
     *      a.isa<binary>() == false
     * */
    template <typename T2>
    bool isa() const noexcept {
        static_assert(is_base_of_t<T, typename T2::type>::value,
                      "Cannot cast T to T2");
        return impl->NodeType() == T2::type::type_code;
    }

    /**
     * Checks if the contained pointer is an instance of type T2::type or its
     * subclass. Will be slower than `isa()` because it uses dynamic_cast
     * `defined()` of this must be `true`.
     *
     * @tparam T2 the type to check. T2 should be NodePtr<X, Base> or its alias
     * like `for_loop`
     * @return `true` if the contained pointer is of type T2. Will check if the
     *  pointer is a sub-class of T2.
     * */
    template <typename T2>
    bool instanceof() const noexcept {  // NOLINT
        return dynamic_cast<typename T2::type*>(impl.get()) != nullptr;
    }

    /**
     * Converts the pointer to T2. Like static_cast, it will not check if the
     * cast is really valid and it is up to the user of this function to ensure
     * the pointer can be casted to T2.
     *
     * @tparam T2 the target type to cast. It should be NodePtr<X, Base>
     * or its alias like `for_loop`. Also the contained type (T2::type) must be
     * a base class or a subclass of T.
     * @return The casted NodePtr of T2.
     * */
    template <typename T2>
    NodePtr<typename T2::type, Base> static_as() const noexcept {
        static_assert(is_base_of_t<T, typename T2::type>::value ||
                          is_base_of_t<typename T2::type, T>::value,
                      "Cannot cast T to T2");
        return NodePtr<typename T2::type, Base>(impl);
    }
    /**
     * Converts the pointer to T2. It will return an empty NodePtr if the
     * contained pointer is not `exactly` T2::type. It uses `isa()` internally.
     * `defined()` of this must be `true`.
     * @see isa
     *
     * @tparam T2 the target type to cast. It should be NodePtr<X, Base> or its
     * alias like `for_loop`.
     * @return The casted NodePtr of T2. If the type of the contained
     * pointer is not T2::type, returns empty
     * */
    template <typename T2>
    NodePtr<typename T2::type, Base> as() const noexcept {
        if (isa<T2>()) {
            return static_as<T2>();
        } else {
            return NodePtr<typename T2::type, Base>();
        }
    }

    /**
     * Converts the pointer to T2. It will abort if the contained
     * pointer is not `exactly` T2::type. It uses `isa()` internally.
     * `defined()` of this must be `true`.
     * @see isa
     *
     * @tparam T2 the target type to cast. It should be NodePtr<X, Base> or its
     * alias like `for_loop`.
     * @return The casted NodePtr of T2.
     * */
    template <typename T2>
    NodePtr<typename T2::type, Base> checked_as() const {
        assert(isa<T2>() && "checked_as failed");
        return static_as<T2>();
    }

    /**
     * Converts the pointer to T2. It will return an empty NodePtr if the
     * contained pointer is not a subclass of T2::type. It uses `instanceof()`
     * internally. `defined()` of this must be `true`.
     * @see instanceof
     *
     * @tparam T2 the target type to cast. It should be NodePtr<X, Base> or its
     * alias like `for_loop`.
     * @return The casted NodePtr of T2. If the type of the contained
     * pointer is not a subclass of T2::type, returns empty
     * */
    template <typename T2>
    NodePtr<typename T2::type, Base> dyn_as() const noexcept {
        if (instanceof<T2>()) {
            return static_as<T2>();
        } else {
            return NodePtr<typename T2::type, Base>();
        }
    }

    /**
     * @brief Try to downcast to a subclass pointer. If not successful, return
     * an empty optional
     *
     * @tparam T2 a subclass pointer type
     * @return optional<NodePtr<typename T2::type, Base>>
     */
    template <typename T2>
    std::optional<NodePtr<typename T2::type, Base>> cast() const {
        return as<T2>();
    }

    /**
     * Adds a const-qualifier to the type T
     * @return The casted NodePtr
     * */
    NodePtr<const T, Base> to_const() const noexcept {
        return NodePtr<const T, Base>(impl);
    }

    /**
     * Removes the const-qualifier of T, like const_cast
     * @return The casted NodePtr
     * */
    NodePtr<typename std::remove_const<T>::type, Base> remove_const()
        const noexcept {
        return NodePtr<typename std::remove_const<T>::type, Base>(impl);
    }

    // operator *
    T& operator*() const noexcept { return *get(); }
    // operator ->
    T* operator->() const noexcept { return get(); }
    // gets the contained pointer
    T* get() const noexcept { return static_cast<T*>(impl.get()); }

    /**
     * Checks if the NodePtr contains any pointer
     * @return false if the NodePtr is empty
     * */
    bool defined() const noexcept { return impl.operator bool(); }

    /**
     * Checks if the NodePtr contains the same pointer of another
     * @param v the other NodePtr to compare with
     * @return true if the NodePtrs are the same
     * */
    bool ptr_same(const NodePtrImpl& v) const noexcept {
        return v.impl == impl;
    }

    /**
     * Gets the weak_ptr of the pointer
     * */
    std::weak_ptr<Base> weak() const noexcept { return impl; }
};

template <typename T, typename Base>
class NodePtr : public NodePtrImpl<T, Base> {
   public:
    using parent = NodePtrImpl<T, Base>;
    using impl_ptr = typename parent::impl_ptr;
    using parent::parent;
    using type = typename parent::type;
    using parent::operator=;
    using parent::operator*;
    using parent::operator->;
    NodePtr() = default;
};

/**
 * The base class of the data-nodes which uses node_ptr as their pointer
 * container. It enables users to get the node_ptr from `this`
 * */
template <typename Base>
class enable_node_ptr_from_this_t : public std::enable_shared_from_this<Base> {
   public:
    /**
     * Get the node_ptr from `this`
     * @return the node_ptr, of which the contained pointer is this
     * */
    NodePtr<Base, Base> node_ptr_from_this() {
        return NodePtr<Base, Base>(
            std::enable_shared_from_this<Base>::shared_from_this());
    }

    /**
     * Get the node_ptr from `this`. Const version
     * @return the node_ptr, of which the contained pointer is this
     * */
    NodePtr<const Base, Base> node_ptr_from_this() const {
        return NodePtr<const Base, Base>(std::const_pointer_cast<Base>(
            std::enable_shared_from_this<Base>::shared_from_this()));
    }
};

class NodeBase {
   public:
    NodeBase();
    virtual ~NodeBase() = default;
    tir::NodeType NodeType() const { return node_type_; }

   protected:
    tir::NodeType node_type_;
};

}  // namespace tir
}  // namespace tdc
