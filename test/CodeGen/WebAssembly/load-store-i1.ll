; RUN: llc < %s -asm-verbose=false | FileCheck %s

; Test that i1 extending loads and truncating stores are assembled properly.

target datalayout = "e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-unknown"

; CHECK-LABEL: load_u_i1_i32:
; CHECK:      load_u_i8_i32 @1{{$}}
; CHECK-NEXT: set_local @2, pop{{$}}
; CHECK-NEXT: return @2{{$}}
define i32 @load_u_i1_i32(i1* %p) {
  %v = load i1, i1* %p
  %e = zext i1 %v to i32
  ret i32 %e
}

; CHECK-LABEL: load_s_i1_i32:
; CHECK:      load_u_i8_i32 @1{{$}}
; CHECK-NEXT: set_local @2, pop{{$}}
; CHECK-NEXT: i32.const 31{{$}}
; CHECK-NEXT: set_local @3, pop{{$}}
; CHECK-NEXT: shl @2, @3{{$}}
; CHECK-NEXT: set_local @4, pop{{$}}
; CHECK-NEXT: shr_s @4, @3{{$}}
; CHECK-NEXT: set_local @5, pop{{$}}
; CHECK-NEXT: return @5{{$}}
define i32 @load_s_i1_i32(i1* %p) {
  %v = load i1, i1* %p
  %e = sext i1 %v to i32
  ret i32 %e
}

; CHECK-LABEL: load_u_i1_i64:
; CHECK:      load_u_i8_i64 @1{{$}}
; CHECK-NEXT: set_local @2, pop{{$}}
; CHECK-NEXT: return @2{{$}}
define i64 @load_u_i1_i64(i1* %p) {
  %v = load i1, i1* %p
  %e = zext i1 %v to i64
  ret i64 %e
}

; CHECK-LABEL: load_s_i1_i64:
; CHECK:      load_u_i8_i64 @1{{$}}
; CHECK-NEXT: set_local @2, pop{{$}}
; CHECK-NEXT: i64.const 63{{$}}
; CHECK-NEXT: set_local @3, pop{{$}}
; CHECK-NEXT: shl @2, @3{{$}}
; CHECK-NEXT: set_local @4, pop{{$}}
; CHECK-NEXT: shr_s @4, @3{{$}}
; CHECK-NEXT: set_local @5, pop{{$}}
; CHECK-NEXT: return @5{{$}}
define i64 @load_s_i1_i64(i1* %p) {
  %v = load i1, i1* %p
  %e = sext i1 %v to i64
  ret i64 %e
}

; CHECK-LABEL: store_i32_i1:
; CHECK:      i32.const 1{{$}}
; CHECK-NEXT: set_local @4, pop{{$}}
; CHECK-NEXT: and @3, @4{{$}}
; CHECK-NEXT: set_local @5, pop{{$}}
; CHECK-NEXT: store_i8 @2, @5{{$}}
define void @store_i32_i1(i1* %p, i32 %v) {
  %t = trunc i32 %v to i1
  store i1 %t, i1* %p
  ret void
}

; CHECK-LABEL: store_i64_i1:
; CHECK:      i64.const 1{{$}}
; CHECK-NEXT: set_local @4, pop{{$}}
; CHECK-NEXT: and @3, @4{{$}}
; CHECK-NEXT: set_local @5, pop{{$}}
; CHECK-NEXT: store_i8 @2, @5{{$}}
define void @store_i64_i1(i1* %p, i64 %v) {
  %t = trunc i64 %v to i1
  store i1 %t, i1* %p
  ret void
}
