#ifndef _GENERIC_REGISTRY_H_
#define _GENERIC_REGISTRY_H_

#include <SimpleTools/SimpleTools.h>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <mutex>

/**
 * @file GenericRegistry.h
 * @brief Generic factory registry system for polymorphic object creation
 *
 * This file implements a comprehensive registry pattern that provides automatic
 * registration and factory-based creation of polymorphic objects. The system
 * is designed to be type-safe, thread-safe, and easy to use through macros
 * that enable automatic registration at static initialization time.
 *
 * @section overview Overview
 *
 * The GenericRegistry system consists of three main components:
 * 1. GenericRegistry<BaseType, EnumType> - The main registry class
 * 2. AutoRegistration<BaseType, EnumType> - RAII registration helper
 * 3. Registry-specific macros - Convenience macros for automatic registration
 *
 * @section design_patterns Design Patterns Used
 *
 * - **Singleton Pattern**: Each registry type has a single global instance
 * - **Factory Pattern**: Creates objects through registered factory functions
 * - **Registry Pattern**: Maintains a collection of available implementations
 * - **RAII Pattern**: Automatic registration during static initialization
 *
 * @section thread_safety Thread Safety
 *
 * The registry is thread-safe for:
 * - Instance creation (GetInstance with autoCreate=true)
 * - Object creation (CreateInstance)
 * - Type querying (IsTypeAvailable, GetAvailableTypes)
 *
 * Registration should happen during static initialization before main().
 *
 * @see See the implementation guide at the end of this file.
 *
 * @tparam BaseType The base class type
 * @tparam EnumType The enum type used as keys
 */
template <typename BaseType, typename EnumType>
class GenericRegistry
{
   public:
    /// Type alias for factory function pointers
    using FactoryPtr = std::unique_ptr<BaseType> (*)();

    // Prevent copying
    GenericRegistry() = default;
    DELETE_COPY_AND_ASSIGNMENT(GenericRegistry);

    /**
     * @brief Get or create the singleton registry instance
     *
     * This method implements the singleton pattern with thread-safe lazy initialization.
     * When autoCreate is true, the instance will be created if it doesn't exist.
     *
     * @param autoCreate If true, create the instance if it doesn't exist
     * @return Pointer to the singleton instance, or nullptr if not created and autoCreate is false
     *
     * @note This method is thread-safe when autoCreate=true
     *
     * @warning The returned pointer should not be stored long-term. Always call
     *          GetInstance() when you need to access the registry.
     */
    static GenericRegistry* GetInstance(bool autoCreate = false) noexcept
    {
        if (autoCreate && !m_instance)
        {
            std::call_once(m_initFlag,
                           []()
                           {
                               // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,bugprone-unhandled-exception-at-new)
                               m_instance = new GenericRegistry();
                           });
        }
        return m_instance;
    }

    /**
     * @brief Set the singleton instance (primarily for testing)
     *
     * This method allows setting a custom registry instance, which is useful
     * for unit testing or dependency injection scenarios.
     *
     * @param instance Pointer to the registry instance to use
     */
    static void SetInstance(GenericRegistry* instance) noexcept { m_instance = instance; }

    /**
     * @brief Register an enum type with its factory function
     *
     * This method associates an enum value with a factory function that can
     * create instances of the corresponding implementation. Typically called
     * automatically through registration macros during static initialization.
     *
     * @param enumType The enum value this factory implements
     * @param factory Function pointer that creates instances of this type
     *
     * @note This method is not thread-safe. Registration should happen during
     *       static initialization before main() to avoid race conditions.
     */
    void RegisterFactory(EnumType enumType, FactoryPtr factory) { m_factories[enumType] = factory; }

    /**
     * @brief Create an instance for the specified enum type
     *
     * This is the main factory method that creates polymorphic objects based
     * on the registered enum type. The method looks up the factory function
     * associated with the enum value and calls it to create a new instance.
     *
     * @param enumType The enum type to create an instance for
     * @return Unique pointer to the newly created instance
     *
     * @throws std::runtime_error if no factory is registered for the type
     *
     * @note The returned object is guaranteed to be non-null if no exception is thrown
     * @note Each call creates a new instance - this is not a singleton pattern
     */
    std::unique_ptr<BaseType> CreateInstance(EnumType enumType)
    {
        auto it = m_factories.find(enumType);
        if (it == m_factories.end())
        {
            throw std::runtime_error("No factory registered for type: " + std::to_string(TOINT(enumType)));
        }

        return it->second();
    }

    /**
     * @brief Get list of all registered enum types
     *
     * Returns a sorted vector containing all enum values that have registered
     * factory functions. The list is sorted by the integer value of the enums
     * to provide consistent ordering.
     *
     * @return Vector of available enum types, sorted by integer value
     *
     * @note The returned vector is a copy, so modifying it won't affect the registry
     * @note This method is thread-safe for reading operations
     * @see IsTypeAvailable() to check for a specific type
     */
    std::vector<EnumType> GetAvailableTypes() const
    {
        std::vector<EnumType> types;
        types.reserve(m_factories.size());

        for (const auto& pair : m_factories)
        {
            types.push_back(pair.first);
        }

        std::sort(types.begin(), types.end(), [](EnumType a, EnumType b) { return TOINT(a) < TOINT(b); });

        return types;
    }

    /**
     * @brief Check if a specific enum type is available
     *
     * This method provides a fast way to check whether a factory function
     * has been registered for a specific enum value without triggering
     * object creation or exception throwing.
     *
     * @param enumType The type to check for availability
     * @return True if the type is registered and can be instantiated, false otherwise
     *
     * @note This method is thread-safe for reading operations
     * @note Prefer this method over try-catch with CreateInstance() for availability checks
     * @see CreateInstance() to actually create an instance
     * @see GetAvailableTypes() to get all available types
     */
    bool IsTypeAvailable(EnumType enumType) const { return m_factories.find(enumType) != m_factories.end(); }

    /**
     * @brief Shutdown the registry and clean up resources
     *
     * This static method destroys the singleton instance and releases any
     * associated resources. It's primarily useful for clean shutdown in
     * applications that need to explicitly manage memory or in testing
     * scenarios where registry state needs to be reset.
     *
     * @note After calling Shutdown(), GetInstance() will return nullptr unless
     *       called with autoCreate=true
     * @note This method is not automatically called at program termination
     * @note Not thread-safe - should only be called during controlled shutdown
     *
     * @warning Once shut down, all previously obtained registry pointers become invalid
     */
    static void Shutdown()
    {
        auto registry = GenericRegistry::GetInstance();
        if (registry)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            delete registry;
            GenericRegistry::SetInstance(nullptr);
        }
    }

   private:
    std::unordered_map<EnumType, FactoryPtr> m_factories;

    static GenericRegistry* m_instance;  ///< Singleton instance pointer
    static std::once_flag m_initFlag;    ///< Thread-safe initialization flag
};

// Static member definitions
template <typename BaseType, typename EnumType>
GenericRegistry<BaseType, EnumType>* GenericRegistry<BaseType, EnumType>::m_instance = nullptr;

template <typename BaseType, typename EnumType>
std::once_flag GenericRegistry<BaseType, EnumType>::m_initFlag;

/**
 * @brief RAII helper class for automatic factory registration
 *
 * This class implements automatic registration using the RAII pattern.
 * When an instance of this class is created (typically as a static global
 * variable), it automatically registers a factory function with the registry
 * during static initialization time.
 *
 * @note This class is typically not used directly. Instead, use the
 *       registry-specific macros (e.g., REGISTER_SOLVER, REGISTER_PLUGIN)
 *       which create instances of this class automatically.
 *
 * @section registration_mechanics Registration Mechanics
 *
 * The registration happens during C++ static initialization, which occurs
 * before main() is called. This ensures that all implementations are
 * registered and available when the program starts.
 *
 * @section static_initialization_order Static Initialization Order
 *
 * C++ guarantees that static variables within the same translation unit
 * are initialized in the order of their definition. However, the order
 * across different translation units is undefined. This registry system
 * handles this correctly by:
 *
 * 1. Creating the registry singleton on first access
 * 2. Using std::call_once for thread-safe initialization
 * 3. Registering factories immediately when AutoRegistration instances are created
 *
 * @tparam BaseType The base class type managed by the registry
 * @tparam EnumType The enum type used as registry keys
 */
template <typename BaseType, typename EnumType>
class AutoRegistration
{
   public:
    /// Type alias for the registry this registration works with
    using RegistryType = GenericRegistry<BaseType, EnumType>;
    /// Type alias for factory function pointers
    using FactoryPtr = typename RegistryType::FactoryPtr;

    /**
     * @brief Constructor that performs the automatic registration
     *
     * When this constructor is called (during static initialization),
     * it automatically gets or creates the registry singleton and
     * registers the provided factory function for the specified enum type.
     *
     * @param enumType The enum type this factory implements
     * @param factory Factory function that creates instances of the implementation
     *
     * @note This constructor is called during static initialization, before main()
     * @note The registry is created automatically if it doesn't exist yet
     * @note Registration is immediate - no deferred initialization
     */
    AutoRegistration(EnumType enumType, FactoryPtr factory)
    {
        auto registry = RegistryType::GetInstance(true);
        registry->RegisterFactory(enumType, factory);
    }

    // Prevent copying
    DELETE_COPY_AND_ASSIGNMENT(AutoRegistration);
};

/**
 * @section implementation_guide Implementation Guide
 *
 * @subsection creating_new_registry Creating a New Registry Type
 *
 * To create a new registry for your polymorphic hierarchy:
 *
 * 1. **Define your base interface:**
 * @code{.cpp}
 * class MyInterface {
 * public:
 *     virtual ~MyInterface() = default;
 *     virtual void DoSomething() = 0;
 * };
 * @endcode
 *
 * 2. **Define your enum for different implementations:**
 * @code{.cpp}
 * enum class MyImplementationType {
 *     Implementation1 = 0,
 *     Implementation2 = 1,
 *     Implementation3 = 2
 * };
 * @endcode
 *
 * 3. **Create a registry header (MyRegistry.h):**
 * @code{.cpp}
 * #ifndef _MY_REGISTRY_H_
 * #define _MY_REGISTRY_H_
 *
 * #include <SimpleTools/GenericRegistry.h>
 * #include "MyInterface.h"
 *
 * using MyRegistry = GenericRegistry<MyInterface, MyImplementationType>;
 * using MyAutoRegistration = AutoRegistration<MyInterface, MyImplementationType>;
 *
 * #define REGISTER_MY_IMPLEMENTATION(ImplClass, ImplType) \
 *     namespace { \
 *         std::unique_ptr<MyInterface> CreateMyImpl() { \
 *             return std::make_unique<ImplClass>(); \
 *         } \
 *         const MyAutoRegistration g_autoRegistration(ImplType, CreateMyImpl); \
 *     }
 *
 * #endif
 * @endcode
 *
 * 4. **Implement your concrete classes:**
 * @code{.cpp}
 * // Implementation1.cpp
 * #include "MyRegistry.h"
 *
 * class Implementation1 : public MyInterface {
 * public:
 *     void DoSomething() override {
 *         // Implementation here
 *     }
 * };
 *
 * REGISTER_MY_IMPLEMENTATION(Implementation1, MyImplementationType::Implementation1);
 * @endcode
 *
 * 5. **Use the registry:**
 * @code{.cpp}
 * #include "MyRegistry.h"
 *
 * void UseImplementation() {
 *     auto registry = MyRegistry::GetInstance();
 *     if (registry && registry->IsTypeAvailable(MyImplementationType::Implementation1)) {
 *         auto impl = registry->CreateInstance(MyImplementationType::Implementation1);
 *         impl->DoSomething();
 *     }
 * }
 * @endcode
 *
 * @subsection macro_design Macro Design Guidelines
 *
 * When creating registration macros:
 *
 * - Use descriptive names that indicate what you're registering
 * - Include the registry type in the macro name for clarity
 * - Use anonymous namespaces to avoid symbol conflicts
 * - Make factory function names descriptive but unique
 * - Use const for the registration object to prevent modification
 */

#endif
