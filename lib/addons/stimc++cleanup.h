/*
 *  stimc is a lightweight verilog-vpi wrapper for stimuli generation.
 *  Copyright (C) 2019-2022  Andreas Dixius, Felix Neumärker
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/**
 * @file
 * @brief stimc++ memory cleanup helpers inspired by c++ memory smart pointers.
 */

#ifndef STIMCXXCLEANUP_H
#define STIMCXXCLEANUP_H

#include <stimc++.h>

/**
 * @brief stimc++ namespace.
 */
namespace stimcxx {
    class cleanup_ptr_base {
        private:
            class ptr_thread_cleanup : public thread_cleanup {
                private:
                    cleanup_ptr_base *_managed_ptr;

                public:
                    ptr_thread_cleanup (cleanup_ptr_base *managed_ptr) noexcept :
                        _managed_ptr (managed_ptr)
                    {}

                    ptr_thread_cleanup            (const ptr_thread_cleanup &p) = delete; /**< @brief Do not copy/change internals */
                    ptr_thread_cleanup& operator= (const ptr_thread_cleanup &p) = delete; /**< @brief Do not copy/change internals */
                    ptr_thread_cleanup            (ptr_thread_cleanup &&p)      = delete; /**< @brief Do not move/change internals */
                    ptr_thread_cleanup& operator= (ptr_thread_cleanup &&p)      = delete; /**< @brief Do not move/change internals */

                    virtual ~ptr_thread_cleanup ()
                    {
                        if (_managed_ptr != nullptr) {
                            _managed_ptr->cleanup ();
                            _managed_ptr->_thread_cleanup = nullptr;
                        }
                    }

                    void clear () noexcept
                    {
                        _managed_ptr = nullptr;
                    }
            };

        protected:
            ptr_thread_cleanup *_thread_cleanup;
            void *_ptr;

        public:
            cleanup_ptr_base (void *ptr = nullptr) :
                _thread_cleanup (new ptr_thread_cleanup (this)),
                _ptr (ptr)
            {}

            cleanup_ptr_base            (const cleanup_ptr_base &c) = delete; /**< @brief Do not copy/change internals */
            cleanup_ptr_base& operator= (const cleanup_ptr_base &c) = delete; /**< @brief Do not copy/change internals */

        protected:
            cleanup_ptr_base (cleanup_ptr_base &&c) noexcept :
                _thread_cleanup (c._thread_cleanup),
                _ptr (c._ptr)
            {
                c._thread_cleanup = nullptr;
                c._ptr            = nullptr;
            }

            cleanup_ptr_base& operator= (cleanup_ptr_base &&c) noexcept
            {
                swap (c);
                return *this;
            }

        public:
            virtual ~cleanup_ptr_base ()
            {
                if (_thread_cleanup != nullptr) _thread_cleanup->clear ();
            }

        protected:
            virtual void cleanup () = 0;

            void reset (void *ptr = nullptr)
            {
                cleanup ();
                this->_ptr = ptr;
            }

            void swap (cleanup_ptr_base &other) noexcept
            {
                void *tmp = this->_ptr;

                this->_ptr = other._ptr;
                other._ptr = tmp;
            }

            void *release () noexcept
            {
                void *tmp = this->_ptr;

                this->_ptr = nullptr;
                return tmp;
            }
            // TODO: doxy-comments
            // TODO: typechecks (isarray, ....)
    };

    template<class T> class cleanup_ptr : public cleanup_ptr_base {
        protected:
            static T *retype (void *p)
            {
                T *tptr = static_cast<T *>(p);

                return tptr;
            }

            void cleanup () override
            {
                if (_ptr != nullptr) delete retype (_ptr);
                _ptr = nullptr;
            }
        public:
            cleanup_ptr (T *ptr                           = nullptr) : cleanup_ptr_base (ptr) {}
            cleanup_ptr            (const cleanup_ptr &c) = delete;     /**< @brief Do not copy/change internals */
            cleanup_ptr& operator= (const cleanup_ptr &c) = delete;     /**< @brief Do not copy/change internals */

            cleanup_ptr            (cleanup_ptr &&c) noexcept = default;     /**< @brief allow move */
            cleanup_ptr& operator= (cleanup_ptr &&c) noexcept = default;     /**< @brief allow move */

            ~cleanup_ptr ()
            {
                cleanup ();
            }

            T& operator*  () noexcept {return *retype (_ptr);}
            T& operator-> () noexcept {return *retype (_ptr);}

            void reset   (T *ptr = nullptr)            {cleanup_ptr_base::reset (ptr);}
            void swap    (cleanup_ptr &other) noexcept {cleanup_ptr_base::swap  (other);}
            T *release ()                   noexcept {return retype (cleanup_ptr_base::release ());}
    };

    template<class T> class cleanup_ptr<T[]> : public cleanup_ptr_base {
        protected:
            static T *retype (void *p)
            {
                T *tptr = static_cast<T *>(p);

                return tptr;
            }

            void cleanup () override
            {
                if (_ptr != nullptr) delete[] retype (_ptr);
                _ptr = nullptr;
            }
        public:
            cleanup_ptr (T *ptr                           = nullptr) : cleanup_ptr_base (ptr) {}
            cleanup_ptr            (const cleanup_ptr &c) = delete;     /**< @brief Do not copy/change internals */
            cleanup_ptr& operator= (const cleanup_ptr &c) = delete;     /**< @brief Do not copy/change internals */

            cleanup_ptr            (cleanup_ptr &&c) noexcept = default;     /**< @brief Allow move */
            cleanup_ptr& operator= (cleanup_ptr &&c) noexcept = default;     /**< @brief Allow move */

            ~cleanup_ptr ()
            {
                cleanup ();
            }

            T& operator[] (std::size_t idx) noexcept {return retype (_ptr)[idx];}

            void reset   (T *ptr = nullptr)            {cleanup_ptr_base::reset (ptr);}
            void swap    (cleanup_ptr &other) noexcept {cleanup_ptr_base::swap  (other);}
            T *release ()                   noexcept {return retype (cleanup_ptr_base::release ());}
    };
}

#endif

