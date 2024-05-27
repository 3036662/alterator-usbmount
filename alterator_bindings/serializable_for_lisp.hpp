/* File: serializable_for_lisp.hpp  

  Copyright (C)   2024
  Author: Oleg Proskurin, <proskurinov@basealt.ru>
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <https://www.gnu.org/licenses/>. 

*/

#pragma once

#include "types.hpp"

/// @brief CRTP base class for serializable objects
/// @tparam Impl Implementation class
template <typename Impl> class SerializableForLisp {
public:
  vecPairs SerializeForLisp() const {
    return static_cast<const Impl *>(this)->SerializeForLisp();
  }
};

template <> class SerializableForLisp<vecPairs> {
  vecPairs vec;
public:
  explicit SerializableForLisp(vecPairs &&vec_) : vec{std::move(vec_)} {};
  vecPairs SerializeForLisp() const { return vec; }
};

/// @brief Interface for polimorphic classes 
class ISerializableForLisp{
public:
  virtual vecPairs SerializeForLisp() const noexcept=0;
};