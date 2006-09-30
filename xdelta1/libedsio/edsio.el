;; -*- Emacs-Lisp -*-

(require 'cl)
(require 'pp)

(eval-and-compile (setq load-path (cons ".." (cons "." load-path))))
(provide 'edsio)
(eval-and-compile (setq load-path (cdr (cdr load-path))))

;; Turn of assertions in compiled code.
(eval-and-compile
  (setq cl-optimize-speed 3)
  (setq cl-optimize-safety 1)
  )

;; Begin

(defconst *definition-state* nil
  "List of all the names of variables containing state from the
definition file so that operations may be performed on everything in
the definition file.")
(defconst *definition-attrs* nil
  "List of attributes for sharing indices.")
(defconst *all-objects* nil)
(defconst *output-files* nil
  "List of lists (NAME BUFFER) used during output of headers.")
(defconst *output-prefix* nil
  "Prefix used for outputing header files.")
(defconst *cpp-extension* "c")

;; This defines several functions and one macro.  The indirection makes
;; it a little bit confusing to read.  It defines the macro DEFDNAME,
;; a function DEFDNAME*, MAKE-DNAME, and a setter and getter for each arg.
(eval-and-compile
(defmacro attr-index (attr)
  `(- (length *definition-attrs*) (length (memq ,attr *definition-attrs*))))

(defmacro defastmacro(dtype args attrs)
  "Defines a macro named DEFDTYPE for defining various AST properties."
  (let ((def-macr  (intern (format "def%s" dtype)))
	(def-func  (intern (format "def%s*" dtype)))
	(make-func (intern (format "make-%s" dtype)))
	(state     (intern (format "*%s-defs*" dtype)))
	(exprs nil))
    (if (not *definition-attrs*)
	(setq *definition-attrs* '(menunode menuline)))
    (let ((fields (append args attrs)))
      (while fields
	(if (not (memq (car fields) *definition-attrs*))
	    (setq *definition-attrs* (append *definition-attrs* (list (car fields)))))
	(setq fields (cdr fields))
	)
      )
    ;; Add it to *definition-state*
    (setq *definition-state* (cons state *definition-state*))
    ;; DEFCONST it
    (setq exprs (cons (list 'defconst state (quote nil)) exprs))
    ;; DEFMACRO DEFDTYPE
    (setq exprs (cons (list 'defmacro
			    def-macr
			    args
			    (append (list 'list (list 'quote def-func))
				    (mapcar (function (lambda (x)
							(list 'list (list 'quote 'quote) x)
							)
						      )
					    args)
				    )
			    )
		      exprs
		      )
	  )
    ;; DEFUN DEFDTYPE*
    (setq exprs (cons (list 'defun
			    def-func
			    args
			    (list 'setq
				  state
				  (list 'cons
					(cons make-func args)
					state
					)
				  )
			    )
		      exprs
		      )
	  )
    ;; MAKE-DTYPE
    (setq exprs (cons (list 'defun
			    make-func
			    args
			    (list 'let (list (list 'it (list 'make-vector (length *definition-attrs*) nil)))
				  (if args
				      (cons 'progn (mapcar
						    (function
						     (lambda (x)
						       (list 'aset 'it (attr-index x) x)
						       )
						     )
						    args
						    )
					    )
				    )
				  (if attrs
				      (cons 'progn (mapcar
						    (function
						     (lambda (x)
						       (list 'aset 'it (attr-index x) nil)
						       )
						     )
						    attrs
						    )
					    )
				    )
				  (if (memq 'menu args)
				      (list 'progn
					    (list 'aset 'it (attr-index 'menunode) (list 'function (intern (format "%s-menunode" dtype))))
					    (list 'aset 'it (attr-index 'menuline) (list 'function (intern (format "%s-menuline" dtype))))
					    )
				    )
				  (list 'cons (list 'quote dtype) 'it)
				  )
			    )
		      exprs
		      )
	  )
    ;; Add the fake arguments:
    (if (memq 'menu args)
	(setq attrs (append (list 'menunode 'menuline) attrs)))
    (setq args (append args attrs))
    (while args
      (let* ((thearg (car args))
	     (arg-set (intern (format "%s-%s-set" dtype thearg)))
	     (arg-get (intern (format "%s-%s-get" dtype thearg))))
	;; DTYPE-ARG-GET
	(setq exprs (cons (list 'defmacro
				(intern (format "%s-%s-get" dtype thearg))
				'(obj)
				(list 'list
				      (list 'quote 'aref)
				      (list 'list (list 'quote 'cdr) 'obj)
				      (attr-index thearg))
				)
			  exprs
			  )
	      )
	;; DTYPE-ARG-SET
	(setq exprs (cons (list 'defmacro
				(intern (format "%s-%s-set" dtype thearg))
				'(obj val)
				(list 'list
				      (list 'quote 'aset)
				      (list 'list (list 'quote 'cdr) 'obj)
				      (attr-index thearg)
				      'val)
				)
			  exprs
			  )
	      )
	)
      (setq args (cdr args))
      )
    ;; To see what it's generating uncomment the next 2 lines.
    ;;(setq message-log-max t)
    ;;(mapcar (function pp) exprs)
    (cons 'progn exprs)
    )
  )


;; This is, as the name suggests, really bogus.  Basically, each DEFASTMACRO
;; call adds to the list *definition-state*.  To compile it, however, it has
;; to be done at compile time, so this macro gets evaluated when being compiled
;; and clears the list.  Then the DEFASTMACRO calls are made, and then DEFCDS
;; is called to define CLEAR-DEFINITION-STATE which resets the list to the
;; compile-time computed value of *definition-state*, it would otherwise be
;; empty when running compiled code.
(defmacro bogus ()
  (setq *definition-state* nil)
  (setq *definition-attrs* nil)
  )

  (bogus)

;; Each DEFASTMACRO statement defines a directive for the definition
;; file along with it's argument names.
(defastmacro sertype      (name number fields transients)  ())
(defastmacro module       (name id header pheader)  ())
(defastmacro import       (name)  (prefix))

(defastmacro event        (name level uargs sargs desc)  ())
(defastmacro etype        (name ctype) ())

(defastmacro prophost     (name letter ctype persist)          (proptypes))
(defastmacro prophosttype (host type) ())

(defmacro defcds ()
  (let ((exprs nil))
    (setq exprs (list (list 'defun 'clear-definition-state nil
			    '(setq *all-objects* nil)
			    (list 'setq '*definition-state* (list 'quote *definition-state*))
			    (list 'setq '*definition-attrs* (list 'quote *definition-attrs*))
			    '(mapcar (function (lambda (x) (set x nil))) *definition-state*)
			    )

		      )
	  )
    (mapcar
     (function
      (lambda (x)
	(setq exprs (cons (list 'defmacro
				(intern (format "obj-%s-get" x))
				'(obj)
				(list 'list
				      (list 'quote 'aref)
				      (list 'list (list 'quote 'cdr) 'obj)
				      (attr-index x))
				)
			  exprs
			  )
	      )
	(setq exprs (cons (list 'defmacro
				(intern (format "obj-%s-set" x))
				'(obj val)
				(list 'list
				      (list 'quote 'aset)
				      (list 'list (list 'quote 'cdr) 'obj)
				      (attr-index x)
				      'val)
				)
			  exprs
			  )
	      )
	(let ((get (intern (format "obj-%s-get" x))))
	  (setq exprs (cons (list 'defun
				  (intern (format "obj-%s-eq" x))
				  '(val olist)
				  `(let ((ret nil))
				     (while (and (not ret) olist)
				       (if (eq val (,get (car olist)))
					   (setq ret (car olist))
					 )
				       (setq olist (cdr olist))
				       )
				     ret
				     )
				  )
			    exprs
			    )
		)
	  )
	)
      )
     *definition-attrs*
     )
    ;;(setq message-log-max t)
    ;;(mapcar (function pp) exprs)
    (cons 'progn exprs)
    )
  )

(defcds)
)
;; Entry Points

(defun generate-ser-noargs ()
  (interactive)
  (generate-ser "edsio.ser" "edsio")
  )

(defun generate-ser (input-file output-prefix)
  ;(interactive "finput: \nsoutput: \nsid: ")
  (let ((make-backup-files nil)
	(executing-kbd-macro t))
    (clear-definition-state)

    (do-it input-file output-prefix)
    )
  )

(defconst *library-id* nil
  "Identifier of this library.")

(defconst *library-header* nil
  "Header of this library.")
(defconst *library-pheader* nil
  "Header of this library.")

(defconst *prefix-with-library-header* t)

(defun load-defs(file)
  (load-file file)
  (setq *import-defs* (reverse *import-defs*))
  (setq *module-defs* (reverse *module-defs*))
  (setq *sertype-defs* (reverse *sertype-defs*))

  (setq *event-defs* (reverse *event-defs*))
  (setq *etype-defs* (reverse *etype-defs*))

  (setq *prophost-defs* (reverse *prophost-defs*))
  (setq *prophosttype-defs* (reverse *prophosttype-defs*))
  )

(defconst *header-typedef-marker* nil)
(defconst *source-init-marker* nil)
(defconst *source-top-marker* nil)

(defun do-it (input-file output-prefix)
  (setq *output-files* nil)
  (setq *output-prefix* output-prefix)

  (load-defs input-file)

  (if (not *module-defs*)
      (error "no defmodule in %s" input-file))

  (if (> (length *module-defs*) 1)
      (error "too many defmodules in %s" input-file))

  (setq *library-id* (module-id-get (car *module-defs*)))
  (setq *library-header* (module-header-get (car *module-defs*)))
  (setq *library-pheader* (module-pheader-get (car *module-defs*)))

  (when (not *library-header*)
    (setq *prefix-with-library-header* nil)
    (setq *library-header* (format "%s_edsio.h" *output-prefix*))
    )

  (if (or (<= *library-id* 0)
	  (>= *library-id* 256))
      (error "Library-id is out of range"))

  (if (> (length *sertype-defs*) 24)
      (error "no more than 24 types"))

  (unwind-protect
      (progn

	(output-header-file "_edsio")

	(read-imports)

	(insert "/* Initialize this library. */\n\n")
	(insert "gboolean " *output-prefix* "_edsio_init (void);\n\n")

	(insert "/* Types defined here. */\n\n")
	(setq *header-typedef-marker* (point-marker))

	(insert "/* Functions declared here. */\n\n")

	(output-source-file "_edsio")

	(insert "#include \"" *library-header* "\"\n\n")
	(insert "#include <errno.h>\n\n")

	(if *library-pheader*
	    (insert "#include \"" *library-pheader* "\"\n\n"))

	(insert "/* Declarations. */\n\n")
	(setq *source-top-marker* (point-marker))

	(insert "\n")

	(insert "/* initialize this library. */\n\n")
	(insert "gboolean\n" *output-prefix* "_edsio_init (void)\n{\n")
	(insert "  static gboolean once = FALSE;\n")
	(insert "  static gboolean result = FALSE;\n")
	(insert "  if (once) return result;\n")
	(insert "  once = TRUE;\n")

	(setq *source-init-marker* (point-marker))

	(insert (format "  edsio_library_register (%d, \"%s\");\n" *library-id* *output-prefix*))
	(insert "  result = TRUE;\n")
	(insert "  return TRUE;\n")
	(insert "};\n\n")

 	(if *prophosttype-defs*
 	    (generate-properties))

 	(if *sertype-defs*
 	    (generate-code))

 	(if *event-defs*
 	    (generate-events))

;	(message "source file:\n%s" (buffer-string))

	(mapcar (function (lambda (x) (output-finish-file x))) *output-files*)
	)
    (mapcar (function (lambda (x) (kill-buffer (cadr x)))) *output-files*)
    )
  )

(defvar *all-sertype-defs* nil)
(defvar *all-prophost-defs* nil)
(defvar *all-etype-defs* nil)

(defun read-imports ()

  (setq *all-sertype-defs* *sertype-defs*)
  (setq *all-etype-defs* *etype-defs*)
  (setq *all-prophost-defs* *prophost-defs*)

  (let ((mods *module-defs*)
	(imps0 *import-defs*)
	(imps  *import-defs*)
	(types *sertype-defs*)
	(events *event-defs*)
	(etypes *etype-defs*)
	(phosts *prophost-defs*)
	(phts  *prophosttype-defs*)
	)

    (while imps
      (clear-definition-state)

      (load-defs (import-name-get (car imps)))

      (setq *all-sertype-defs* (append *all-sertype-defs* *sertype-defs*))
      (setq *all-etype-defs* (append *all-etype-defs* *etype-defs*))
      (setq *all-prophost-defs* (append *all-prophost-defs* *prophost-defs*))

      (import-prefix-set (car imps) (module-name-get (car *module-defs*)))

      (when (or *sertype-defs* *event-defs*)
	(output-header-file "_edsio")
	(insert (format "#include \"%s_edsio.h\"\n\n" (import-prefix-get (car imps))))
	)

      (setq imps (cdr imps))
      )

    (setq *module-defs* mods)
    (setq *import-defs* imps0)
    (setq *sertype-defs* types)
    (setq *event-defs* events)
    (setq *etype-defs* etypes)
    (setq *prophost-defs* phosts)
    (setq *prophosttype-defs* phts)
    )
  )


(defun output-header-file (name)
  (output-file (format "%s.h" name) 'c-header *output-prefix*))

(defun output-source-file (name)
  (output-file (format "%s.%s" name *cpp-extension*) 'c *output-prefix*))

(defun output-source-include-file (name)
  (output-file (format "%s.%si" name *cpp-extension*) 'c *output-prefix*))

(defun output-plain-file (name)
  (output-file (format "%s" name) 'plain ""))

(defun output-file (name type prefix)
  (let* ((name (format "%s%s" prefix name))
	 (it (assoc name *output-files*)))
    (if it
	(set-buffer (cadr it))
      (let ((nbuf (get-buffer-create (generate-new-buffer-name name))))
	(setq *output-files* (cons (list name nbuf type) *output-files*))
	(set-buffer nbuf)
	)
      )
    )
  )

(defun output-finish-file (file)
  (let ((name (car file))
	(buf (cadr file))
	(type (caddr file)))
    (set-buffer buf)
    ;(message "printing %s: %s" file (buffer-string))
    (cond ((eq type 'c)
	   (output-to-c name nil))
	  ((eq type 'c-header)
	   (output-to-c name t))
	  )
    (write-file-if-different buf name)
    )
  )

(defun output-to-c (name is-header)
  (goto-char (point-min))
  (insert "/* -*-Mode: C;-*-
 * Copyright (C) 1997, 1998, 1999  Josh MacDonald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 *
 * This file was AUTOMATICALLY GENERATED using:
 *
 * $Id: edsio.el 1.16 Tue, 06 Apr 1999 23:40:10 -0700 jmacd $
 */

")

  (if is-header
      (let ((cppname (string-replace-regexp (upcase name) "[-./]" "_")))
	(insert "#include \"edsio.h\"\n\n")

	(if *prefix-with-library-header*
	    (insert "#include \"" *library-header* "\"\n\n"))

	(insert "#ifndef _" cppname "_\n")
	(insert "#define _" cppname "_\n\n")
	(insert "#ifdef __cplusplus\n")
	(insert "extern \"C\" {\n")
	(insert "#endif\n\n")
	(goto-char (point-max))
	(insert "#ifdef __cplusplus\n")
	(insert "}\n")
	(insert "#endif\n")
	(insert "\n#endif /* _" cppname "_ */\n\n")
	)
    )
  )

(defun string-replace-regexp (str regexp to-string)
  "Result of replacing all occurrences in STR of REGEXP by TO-STRING.  The
replacement is as for replace-regexp."
  (let ((work (get-buffer-create "*string-tmp*")))
    (save-excursion
      (set-buffer work)
      (erase-buffer)
      (insert str)
      (beginning-of-buffer)
      (while (re-search-forward regexp nil t)
	(replace-match to-string nil nil))
      (buffer-string))))

(defun write-file-if-different (buf filename)
  (save-excursion
    (if (not (file-exists-p filename))
	(write-file filename)
      (set-buffer buf)
      (let ((old (get-buffer-create (generate-new-buffer-name filename)))
	    (bmin (point-min))
	    (bmax (point-max)))
	(unwind-protect
	    (progn
	      (set-buffer old)
	      (insert-file filename)
	      (let ((omin (point-min))
		    (omax (point-max))
		    (case-fold-search nil))
		(if (= 0 (compare-buffer-substrings old omin omax buf bmin bmax))
		    (message "Output file %s is unchanged." filename)
		  (set-buffer buf)
		  (write-file filename)
		  )
		)
	      )
	  (kill-buffer old)
	  )
	)
      )
    )
  )


(defun format-comlist (func l)
  (let ((x ""))
    (while l
      (setq x (concat x (funcall func (car l))))
      (if (cdr l)
	  (setq x (concat x ", ")))
      (setq l (cdr l))
      )
    x
    )
  )

(defun format-semilist (func l)
  (let ((x ""))
    (while l
      (setq x (concat x (funcall func (car l))))
      (if (cdr l)
	  (setq x (concat x "; ")))
      (setq l (cdr l))
      )
    x
    )
  )

(defun format-numbered-comlist (func l)
  (let ((x "")
	(n 0))
    (while l
      (setq x (concat x (funcall func (car l) n)))
      (setq n (+ n 1))
      (if (cdr l)
	  (setq x (concat x ", ")))
      (setq l (cdr l))
      )
    x
    )
  )

(defun capitalize1(s)
  (let ((work (get-buffer-create "*string-tmp*")))
    (save-excursion
      (set-buffer work)
      (erase-buffer)
      (insert (format "%s" s))
      (upcase-region (point-min) (+ (point-min) 1))
      (buffer-substring-no-properties (point-min) (point-max))
      )
    )
  )

(defun upcase-string (s)
  (let ((work (get-buffer-create "*string-tmp*")))
    (save-excursion
      (set-buffer work)
      (erase-buffer)
      (insert (format "%s" s))
      (upcase-region (point-min) (point-max))
      (buffer-substring-no-properties (point-min) (point-max))
      )
    )
  )

(defun downcase-string (s)
  (let ((work (get-buffer-create "*string-tmp*")))
    (save-excursion
      (set-buffer work)
      (erase-buffer)
      (insert (format "%s" s))
      (downcase-region (point-min) (point-max))
      (buffer-substring-no-properties (point-min) (point-max))
      )
    )
  )

;; HERE IT IS

(defun generate-code ()

  (let ((all-codes nil))
    (mapcar
     (function
      (lambda (st)
	(let ((x (sertype-number-get st)))
	  (cond ((member x all-codes)
		 (error "serial type number %d defined twice" x))
		((> x 24)
		 (error "serial type value %d too high" x))
		((< x 0)
		 (error "serial type value %d too low" x))
		(t (setq all-codes (cons x all-codes))))))
      )
     *sertype-defs*)
    )

  (output-header-file "_edsio")

  (insert "/* Serial Types */\n\n")
  (insert (format "enum _Serial%sType {\n" (capitalize1 *output-prefix*)))
  (insert (format-comlist
	   (function
	    (lambda (x)
	      (format "\n  ST_%s = (1<<(%d+EDSIO_LIBRARY_OFFSET_BITS))+%d" (sertype-name-get x) (sertype-number-get x) *library-id*))) *sertype-defs*))
  (insert "\n};\n\n")

  (insert "\n\n")

  (output-source-file "_edsio")

  (save-excursion
    (goto-char *source-top-marker*)

    (insert "static void print_spaces (guint n) { int i; for (i = 0; i < n; i += 1) g_print (\" \"); }\n\n")
    )

  (mapcar (function generate-code-entry) *sertype-defs*)

  )

(defun generate-code-entry (entry)
  (let ((ent-upcase   (sertype-name-get entry))
	(ent-downcase (downcase-string (sertype-name-get entry))))

    (output-header-file "_edsio")

    ;; The typedef, structure, and declarations.

    (save-excursion
      (goto-char *header-typedef-marker*)
      (insert (format "typedef struct _Serial%s Serial%s;\n" ent-upcase ent-upcase))
      )

    (insert (format "/* %s Structure\n */\n\n" ent-upcase))

    (insert (format "struct _Serial%s {\n" ent-upcase))

    (apply (function insert)
	   (mapcar (function
		    (lambda (x)
		      (format "  %s;\n" x)))
		   (entry-typename-pairs entry nil)))

    (apply (function insert)
	   (mapcar (function
		    (lambda (x)
		      (format "  %s;\n" x)))
		   (sertype-transients-get entry)))

    (insert "};\n\n")

    (insert (format "void     serializeio_print_%s_obj        (Serial%s* obj, guint indent_spaces);\n\n" ent-downcase ent-upcase))

    (insert (format "gboolean unserialize_%s                  (SerialSource *source, Serial%s**);\n" ent-downcase ent-upcase))
    (insert (format "gboolean unserialize_%s_internal         (SerialSource *source, Serial%s** );\n" ent-downcase ent-upcase))
    (insert (format "gboolean unserialize_%s_internal_noalloc (SerialSource *source, Serial%s* );\n" ent-downcase ent-upcase))
    (insert (format "gboolean serialize_%s                    (SerialSink *sink%s);\n" ent-downcase (entry-arglist t entry)))
    (insert (format "gboolean serialize_%s_obj                (SerialSink *sink, const Serial%s* obj);\n" ent-downcase ent-upcase))
    (insert (format "gboolean serialize_%s_internal           (SerialSink *sink%s);\n" ent-downcase (entry-arglist t entry)))
    (insert (format "gboolean serialize_%s_obj_internal (SerialSink *sink, Serial%s* obj);\n" ent-downcase ent-upcase))
    (insert (format "guint    serializeio_count_%s            (%s);\n" ent-downcase (entry-arglist nil entry)))
    (insert (format "guint    serializeio_count_%s_obj        (Serial%s const* obj);\n" ent-downcase ent-upcase))
    (insert (format "\n"))

    (output-source-file "_edsio")

    ;; The init entry

    (save-excursion
      (goto-char *source-init-marker*)
      (insert (format "  serializeio_initialize_type (\"ST_%s\", ST_%s, &unserialize_%s_internal, &serialize_%s_obj_internal, &serializeio_count_%s_obj, &serializeio_print_%s_obj);\n" ent-upcase ent-upcase ent-downcase ent-downcase ent-downcase ent-downcase))
      )

    ;; Count code

    (insert (format "/* %s Count\n */\n\n" ent-upcase))

    (insert (format "guint\nserializeio_count_%s (%s) {\n" ent-downcase (entry-arglist nil entry)))
    (insert (format "  guint size = sizeof (Serial%s);\n" ent-upcase))
    (apply (function insert)
	   (mapcar (function (lambda (x) (concat
					  (format "  ALIGN_8 (size);\n")
					  (entry-count-field entry x (format "%s" (car x)) "  " t))))
		   (sertype-fields-get entry)))
    (insert (format "  ALIGN_8 (size);\n"))
    (insert (format "  return size;\n"))
    (insert (format "}\n\n"))

    ;; Count Object code

    (insert (format "guint\nserializeio_count_%s_obj (Serial%s const* obj) {\n" ent-downcase ent-upcase))
    (insert (format "  return serializeio_count_%s (%s);\n" ent-downcase (entry-plist t nil "obj->" entry)))
    (insert (format "}\n\n"))

    ;; Print object code

    (insert (format "/* %s Print\n */\n\n" ent-upcase))

    (insert (format "void\nserializeio_print_%s_obj (Serial%s* obj, guint indent_spaces) {\n" ent-downcase ent-upcase))
    (insert (format "  print_spaces (indent_spaces);\n"))

    (insert (format "  g_print (\"[ST_%s]\\n\");\n" ent-upcase))

    (apply (function insert)
	   (mapcar (function (lambda (x) (entry-print-field entry x (format "obj->%s" (car x)) "  " t)))
		   (sertype-fields-get entry)))

    (insert (format "}\n\n"))

    ;; Print internal code

    ;; Serialize code

    (insert (format "/* %s Serialize\n */\n\n" ent-upcase))

    (insert (format "gboolean\nserialize_%s_internal (SerialSink *sink%s)\n" ent-downcase (entry-arglist t entry)))
    (insert (format "{\n"))

    (apply (function insert)
	   (mapcar (function (lambda (x) (entry-serialize-field entry x (format "%s" (car x)) "  " t)))
		   (sertype-fields-get entry)))

    (insert (format "  return TRUE;\n"))
    (if (sertype-fields-get entry)
	(insert (format "bail:\n  return FALSE;\n")))
    (insert (format "}\n\n"))

    ;; Internal Serialize Object code

    (insert (format "gboolean\nserialize_%s_obj_internal (SerialSink *sink, Serial%s* obj)\n" ent-downcase ent-upcase))
    (insert (format "{\n"))
    (insert (format "  return serialize_%s_internal (sink%s);\n" ent-downcase (entry-plist t t "obj->" entry)))
    (insert (format "}\n\n"))

    ;; External Serialize code

    (insert (format "gboolean\nserialize_%s (SerialSink *sink%s)\n" ent-downcase (entry-arglist t entry)))
    (insert (format "{\n"))

    (insert (format "  if (! (* sink->sink_type) (sink, ST_%s, serializeio_count_%s (%s), TRUE)) goto bail;\n" ent-upcase ent-downcase (entry-plist nil nil "" entry)))

    (insert (format "  if (! serialize_%s_internal (sink%s)) goto bail;\n" ent-downcase (entry-plist nil t "" entry)))
    (insert (format "  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;\n"))

    (insert (format "  return TRUE;\n"))
    (insert (format "bail:\n"))
    (insert (format "  return FALSE;\n"))
    (insert (format "}\n\n"))

    ;; External serialize_obj

    (insert (format "gboolean\nserialize_%s_obj (SerialSink *sink, const Serial%s* obj) {\n\n" ent-downcase ent-upcase))
    (insert (format "  return serialize_%s (sink%s);\n" ent-downcase (entry-plist t t "obj->" entry)))
    (insert (format "}\n\n"))

    ;; Unserialize code

    (insert (format "/* %s Unserialize\n */\n\n" ent-upcase))

    (insert (format "gboolean\nunserialize_%s_internal_noalloc (SerialSource *source, Serial%s* result)\n" ent-downcase ent-upcase))
    (insert (format "{\n"))

    (apply (function insert)
	   (mapcar (function (lambda (x) (entry-unserialize-field entry x (format "result->%s" (car x)) "  ")))
		   (sertype-fields-get entry)))

    (insert (format "  return TRUE;\n"))
    (if (sertype-fields-get entry)
	(insert (format "bail:\n  return FALSE;\n")))
    (insert (format "}\n\n"))


    (insert (format "gboolean\nunserialize_%s_internal (SerialSource *source, Serial%s** result)\n" ent-downcase ent-upcase))
    (insert (format "{\n"))

    (insert (format "  Serial%s* unser;\n" ent-upcase))
    (insert (format "  (*result) = NULL;\n"))
    (insert (format "  unser = serializeio_source_alloc (source, sizeof (Serial%s));\n" ent-upcase))
    (insert (format "  if (! unser) goto bail;\n"))

    (insert (format "  if (! unserialize_%s_internal_noalloc (source, unser)) goto bail;\n" ent-downcase))

    (insert (format "  (*result) = unser;\n"))
    (insert (format "  return TRUE;\n"))
    (insert (format "bail:\n"))
    (insert (format "  return FALSE;\n"))
    (insert (format "}\n\n"))

    ;; External unserialize

    (insert (format "gboolean\nunserialize_%s (SerialSource *source, Serial%s** result)\n"  ent-downcase ent-upcase))
    (insert (format "{\n"))

    (insert (format "  if ( (* source->source_type) (source, TRUE) != ST_%s) goto bail;\n" ent-upcase))

    (insert (format "  if (! unserialize_%s_internal (source, result)) goto bail;\n" ent-downcase))

    (insert (format "  if (! serializeio_source_object_received (source)) goto bail;\n"))

    (insert (format "  return TRUE;\n"))
    (insert (format "bail:\n"))
    (insert (format "  return FALSE;\n"))

    (insert (format "}\n\n"))

    )
  )

(defun entry-typename-pairs (entry is-param)
  (let ((pairs nil)
	(fields (sertype-fields-get entry)))
    (while fields
      (let ((field (car fields)))
	(when (or (equal (cadr field) 'bytes)
		  (and (consp (cadr field)) (equal (caadr field) 'array)))
	  (setq pairs (cons (format "guint32 %s_len" (car field)) pairs))
	  )
	(when (equal (cadr field) 'object)
	  (setq pairs (cons (format "guint32 %s_type" (car field)) pairs))
	  )
	(setq pairs (cons (field-decl field is-param) pairs))
	)
      (setq fields (cdr fields))
      )
    (nreverse pairs)
    )
  )

(defun entry-param-names (prefix entry need_pbr)
  (let ((pairs nil)
	(fields (sertype-fields-get entry)))
    (while fields
      (let ((field (car fields)))
	(when (or (equal (cadr field) 'bytes)
		  (and (consp (cadr field)) (equal (caadr field) 'array)))
	  (setq pairs (cons (format "%s%s_len" prefix (car field)) pairs))
	  )
	(when (equal (cadr field) 'object)
	  (setq pairs (cons (format "%s%s_type" prefix (car field)) pairs))
	  )
	(setq pairs (cons (format "%s%s%s" (if (and need_pbr (needs-ref field)) "&" "") prefix (car field)) pairs))
	)
      (setq fields (cdr fields))
      )
    (nreverse pairs)
    )
  )

(defun field-ctype (field)
  (cond ((equal (cadr field) 'string)
	 "const gchar*")
	((equal (cadr field) 'uint)
	 "guint32")
	((equal (cadr field) 'uint32)
	 "guint32")
	((equal (cadr field) 'uint16)
	 "guint16")
	((equal (cadr field) 'uint8)
	 "guint8")
	((equal (cadr field) 'boolean)
	 "gboolean")
	((equal (cadr field) 'bytes)
	 "const guint8*")
	((equal (cadr field) 'object)
	 "void*")
	((member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
	 (format "Serial%s" (cadr field)))
	((equal (car (cadr field)) 'bytes)
	 "const guint8*")
	((member (car (cadr field)) '(array ptr))
	 (concat (field-ctype (cadr field)) "*"))
	(t (error "unrecognized field type: %s" (cadr field))))
  )

(defun field-decl (field is-param)
  (if (and (consp (cadr field))
	   (equal (car (cadr field)) 'bytes))
      (format "%sguint8 %s[%d]" (if is-param "const " "") (car field) (cadr (cadr field)))
    ;(message "foo %s %s" field (member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*)))
    (format "%s %s"
	    (cond ((member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
		   (format "Serial%s%s" (cadr field) (if is-param " const*" "")))
		  ((equal (cadr field) 'string)
		   "const gchar*")
		  ((equal (cadr field) 'uint)
		   "guint32")
		  ((equal (cadr field) 'uint32)
		   "guint32")
		  ((equal (cadr field) 'uint16)
		   "guint16")
		  ((equal (cadr field) 'uint8)
		   "guint8")
		  ((equal (cadr field) 'boolean)
		   "gboolean")
		  ((equal (cadr field) 'bytes)
		   "const guint8*")
		  ((equal (cadr field) 'object)
		   "void*")
		  ((member (car (cadr field)) '(array ptr))
		   (concat (field-ctype (cadr field)) (if is-param " const*" "*")))
		  (t (error "unrecognized field type: %s" (cadr field))))
	    (car field)))
  )

(defun entry-arglist (need_first entry)
  (concat
   (if (and need_first (sertype-fields-get entry)) ", " "")
   (format-comlist (function (lambda (x) x)) (entry-typename-pairs entry t))))

(defun needs-ref (field)
  (member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
  )

(defun entry-plist (need_pbr need_first prefix entry)
  (concat
   (if (and need_first (sertype-fields-get entry)) ", " "")
   (format-comlist (function (lambda (x) (format "%s" x)))
		   (entry-param-names prefix entry need_pbr))))

(defun entry-unserialize-field (entry field name prefix)
  (cond ((equal (cadr field) 'uint)
	 (format "%sif (! (* source->next_uint) (source, &%s)) goto bail;\n" prefix name))
	((equal (cadr field) 'uint32)
	 (format "%sif (! (* source->next_uint32) (source, &%s)) goto bail;\n" prefix name))
	((equal (cadr field) 'uint16)
	 (format "%sif (! (* source->next_uint16) (source, &%s)) goto bail;\n" prefix name))
	((equal (cadr field) 'uint8)
	 (format "%sif (! (* source->next_uint8) (source, &%s)) goto bail;\n" prefix name))
	((equal (cadr field) 'boolean)
	 (format "%sif (! (* source->next_bool) (source, &%s)) goto bail;\n" prefix name))
	((equal (cadr field) 'string)
	 (format "%sif (! (* source->next_string) (source, &%s)) goto bail;\n" prefix name))
	((equal (cadr field) 'bytes)
	 (format "%sif (! (* source->next_bytes) (source, &%s, &%s_len)) goto bail;\n" prefix name name))
	((equal (cadr field) 'object)
	 (format "%sif (! serializeio_unserialize_generic_internal (source, &%s_type, &%s, FALSE)) goto bail;\n" prefix name name))
	((member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
	 (format "%sif (! unserialize_%s_internal_noalloc (source, &%s)) goto bail;\n" prefix (downcase-string (cadr field)) name))
	((and (equal (car (cadr field)) 'ptr)
	      (member (cadr (cadr field)) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*)))
	 (format "%sif (! unserialize_%s_internal (source, &%s)) goto bail;\n" prefix (downcase-string (cadr (cadr field))) name))
	((equal (car (cadr field)) 'bytes)
	 (format "%sif (! (* source->next_bytes_known) (source, %s, %d)) goto bail;\n" prefix name (cadr (cadr field))))
	((equal (car (cadr field)) 'array)
	 (format "%s{
%s  gint i;
%s  if (! (* source->next_uint) (source, &%s_len)) goto bail;
%s  if (! (%s = serializeio_source_alloc (source, sizeof (%s) * %s_len))) goto bail;
%s  for (i = 0; i < %s_len; i += 1)
%s    {
%s%s      }
%s}
"
		 prefix
		 prefix prefix
		 name
		 prefix
		 name
		 (field-ctype (cadr field))
		 name
		 prefix
		 name
		 prefix
		 prefix
		 (entry-unserialize-field entry (cadr field) (concat "(" name "[i])") (concat prefix "    "))
		 prefix
		 ))
	(t (error "unrecognized field type: %s" (cadr field)))))


(defun entry-serialize-field (entry field name prefix is-param)
  (cond ((equal (cadr field) 'uint)
	 (format "%sif (! (* sink->next_uint) (sink, %s)) goto bail;\n" prefix name))
	((equal (cadr field) 'uint16)
	 (format "%sif (! (* sink->next_uint16) (sink, %s)) goto bail;\n" prefix name))
	((equal (cadr field) 'uint8)
	 (format "%sif (! (* sink->next_uint8) (sink, %s)) goto bail;\n" prefix name))
	((equal (cadr field) 'uint32)
	 (format "%sif (! (* sink->next_uint32) (sink, %s)) goto bail;\n" prefix name))
	((equal (cadr field) 'boolean)
	 (format "%sif (! (* sink->next_bool) (sink, %s)) goto bail;\n" prefix name))
	((equal (cadr field) 'string)
	 (format "%sif (! (* sink->next_string) (sink, %s)) goto bail;\n" prefix name))
	((equal (cadr field) 'bytes)
	 (format "%sif (! (* sink->next_bytes) (sink, %s, %s_len)) goto bail;\n" prefix name name))
	((equal (cadr field) 'object)
	 (format "%sif (! serializeio_serialize_generic_internal (sink, %s_type, %s, FALSE)) goto bail;\n" prefix name name))
	((member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
	 (format "%sif (! serialize_%s_internal (sink%s)) goto bail;\n" prefix (downcase-string (cadr field))
		 (entry-plist t t (concat name (if is-param "->" ".")) (obj-name-eq (cadr field) *all-sertype-defs*))))
	((and (equal (car (cadr field)) 'ptr)
	      (member (cadr (cadr field)) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*)))
	 (format "%sif (! serialize_%s_internal (sink%s)) goto bail;\n" prefix (downcase-string (cadr (cadr field)))
		 (entry-plist t t (concat name "->") (obj-name-eq (cadr (cadr field)) *all-sertype-defs*))))
	((equal (car (cadr field)) 'bytes)
	 (format "%sif (! (* sink->next_bytes_known) (sink, %s, %d)) goto bail;\n" prefix name (cadr (cadr field))))
	((equal (car (cadr field)) 'array)
	 (format "%s{
%s  gint i;
%s  if (! (* sink->next_uint) (sink, %s_len)) goto bail;
%s  for (i = 0; i < %s_len; i += 1)
%s    {
%s%s      }
%s}
"
		 prefix prefix prefix
		 name
		 prefix
		 name
		 prefix
		 prefix
		 (entry-serialize-field entry (cadr field) (array-index name (cadr field)) (concat prefix "    ") nil)
		 prefix
		 ))
	(t (error "unrecognized field type: %s" (cadr field)))))

(defun array-index (name field)
  ;(concat "(" (if (needs-ref field) "&" "") name "[i])")
  (concat "(" name "[i])")
  )

(defun entry-count-field (entry field name prefix is-param)
  (cond ((equal (cadr field) 'uint)
	 ;(format "%ssize += sizeof (guint32);\n" prefix)
	 ""
	 )
	((equal (cadr field) 'uint32)
	 ;(format "%ssize += sizeof (guint32);\n" prefix)
	 ""
	 )
	((equal (cadr field) 'uint16)
	 ;(format "%ssize += sizeof (guint16);\n" prefix)
	 ""
	 )
	((equal (cadr field) 'uint8)
	 ;(format "%ssize += sizeof (guint8);\n" prefix)
	 ""
	 )
	((equal (cadr field) 'boolean)
	 ;(format "%ssize += sizeof (gboolean);\n" prefix)
	 ""
	 )
	((equal (cadr field) 'string)
	 (format "%ssize += strlen (%s) + 1;\n" prefix name)
	 )
	((equal (cadr field) 'bytes)
	 (format "%ssize += %s_len;\n" prefix name)
	 )
	((equal (cadr field) 'object)
	 (format "%ssize += serializeio_generic_count (%s_type, %s);\n" prefix name name)
	 )
	((member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
 	 (format "%ssize += serializeio_count_%s_obj (%s%s) - sizeof (Serial%s);\n"
		 prefix
		 (downcase-string (cadr field))
		 (if is-param "" "& ")
		 name
		 (cadr field)
 		 )
	 )
	((and (equal (car (cadr field)) 'ptr)
	      (member (cadr (cadr field)) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*)))
 	 (format "%ssize += serializeio_count_%s_obj (%s);\n" prefix (downcase-string (cadr (cadr field))) name)
	 )
	((equal (car (cadr field)) 'bytes)
	 ;(format "%ssize += 0;\n" prefix (cadr (cadr field)))
	 ""
	 )
	((equal (car (cadr field)) 'array)
	 (format "%s{
%s  gint i;
%s  for (i = 0; i < %s_len; i += 1)
%s    {
%s%s      }
%s}
"
		 prefix prefix prefix
		 name
		 prefix
		 prefix
		 (entry-count-array-field entry (cadr field) (array-index name (cadr field)) (concat prefix "    ") nil)
		 prefix
		 ))
	(t (error "unrecognized field type: %s" (cadr field)))))

(defun entry-count-array-field (entry field name prefix is-param)
  (cond ((equal (cadr field) 'uint)
	 (format "%ssize += sizeof (guint32);\n" prefix)
	 )
	((equal (cadr field) 'uint32)
	 (format "%ssize += sizeof (guint32);\n" prefix)
	 )
	((equal (cadr field) 'uint16)
	 (format "%ssize += sizeof (guint16);\n" prefix)
	 )
	((equal (cadr field) 'uint8)
	 (format "%ssize += sizeof (guint8);\n" prefix)
	 )
	((equal (cadr field) 'boolean)
	 (format "%ssize += sizeof (gboolean);\n" prefix)
	 )
	((equal (cadr field) 'string)
	 (format "%ssize += strlen (%s) + 1 + sizeof (void*);\n" prefix name)
	 )
	((equal (cadr field) 'bytes)
	 (error "can't do that: bytes1")
	 )
	((equal (cadr field) 'object)
	 (error "can't do that: object")
	 )
	((member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
 	 (format "%ssize += serializeio_count_%s_obj (%s%s);\n"
		 prefix
		 (downcase-string (cadr field))
		 (if is-param "" "& ")
		 name
		 (cadr field)
 		 )
	 )
	((and (equal (car (cadr field)) 'ptr)
	      (member (cadr (cadr field)) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*)))
 	 (format "%ssize += serializeio_count_%s_obj (%s) + sizeof (void*);\n" prefix (downcase-string (cadr (cadr field))) name)
	 )
	((equal (car (cadr field)) 'bytes)
	 (error "can't do that: bytes2")
	 )
	((equal (car (cadr field)) 'array)
	 (error "can't do that: array")
	 )
	(t (error "unrecognized field type: %s" (cadr field)))))

(defun entry-print-field (entry field name prefix is-param)
  (concat
   (format "%sprint_spaces (indent_spaces);\n" prefix)
   (if is-param (format "%sg_print (\"%s = \");\n" prefix (car field)) "")
   (cond ((equal (cadr field) 'uint)
	  (format "%sg_print (\"%%d\\n\", %s);\n" prefix name))
	 ((equal (cadr field) 'uint32)
	  (format "%sg_print (\"%%d\\n\", %s);\n" prefix name))
	 ((equal (cadr field) 'uint16)
	  (format "%sg_print (\"%%d\\n\", %s);\n" prefix name))
	 ((equal (cadr field) 'uint8)
	  (format "%sg_print (\"%%d\\n\", %s);\n" prefix name))
	 ((equal (cadr field) 'boolean)
	  (format "%sg_print (\"%%s\\n\", %s ? \"true\" : \"false\");\n" prefix name))
	 ((equal (cadr field) 'string)
	  (format "%sg_print (\"%%s\\n\", %s);\n" prefix name))
	 ((equal (cadr field) 'bytes)
	  (format "%sserializeio_print_bytes (%s, %s_len);\n" prefix name name))
	 ((equal (cadr field) 'object)
	  (concat
 	   (if is-param (format "%sg_print (\"{\\n\");\n" prefix) "")
	   (format "%sserializeio_generic_print (%s_type, %s, indent_spaces + 2);\n" prefix name name)
	   (format "%sprint_spaces (indent_spaces);\n;\n" prefix)
	   (if is-param (format "%sg_print (\"}\\n\");\n" prefix) "")
	   )
	  )
	 ((member (cadr field) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
	  (concat
	   (if is-param (format "%sg_print (\"{\\n\");\n" prefix) "")
	   (format "%sserializeio_print_%s_obj (& %s, indent_spaces + 2);\n" prefix (downcase-string (cadr field)) name name)
	   (format "%sprint_spaces (indent_spaces);\n;\n" prefix)
	   (if is-param (format "%sg_print (\"}\\n\");\n" prefix) "")
	   )
	  )
	 ((and (equal (car (cadr field)) 'ptr)
	       (member (cadr (cadr field)) (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*)))
	  (concat
	   (if is-param (format "%sg_print (\"{\\n\");\n" prefix) "")
	   (format "%sserializeio_print_%s_obj (%s, indent_spaces + 2);\n"
		  prefix (downcase-string (cadr (cadr field))) name name)
	   (format "%sprint_spaces (indent_spaces);\n;\n" prefix)
	   (if is-param (format "%sg_print (\"}\\n\");\n" prefix) "")
	   )
	  )
	 ((equal (car (cadr field)) 'bytes)
	  (format "%sserializeio_print_bytes (%s, %d);\n" prefix name (cadr (cadr field))))
	 ((equal (car (cadr field)) 'array)
	  (concat
	   (if is-param (format "%sg_print (\"{\\n\");\n" prefix) "")
	   (format "%s{
%s  gint i;
%s  for (i = 0; i < %s_len; i += 1)
%s    {
%s      print_spaces (indent_spaces);
%s      g_print (\"%%d:\n\", i);
%s%s      }
%s}
"
		   prefix prefix prefix
		   name
		   prefix
		   prefix
		   prefix
		   prefix
		   (entry-print-field entry (cadr field) (array-index name (cadr field)) (concat prefix "    ") nil)
		   prefix
		   )
	   (if is-param (format "%sg_print (\"}\\n\");\n" prefix) "")))
	 (t (error "unrecognized field type: %s" (cadr field)))))
  )

(defconst *event-id* 0)

(defconst *event-types* nil)

(defun generate-events ()
  (let ((events *event-defs*))
    (while events

      (let* ((event (car events))
	     (uargs (event-uargs-get event))
	     (sargs (event-sargs-get event))
	     (type-prefix (intern (apply (function concat)
					 (append (mapcar (function (lambda (x) (capitalize1 (cadr x)))) uargs)
						 (mapcar (function (lambda (x) (capitalize1 x))) sargs)))))
	     (capprefix (capitalize1 *output-prefix*)))

	(if (and (not uargs) (not sargs))
	    (setq type-prefix "Void"))

	(when (not (member type-prefix *event-types*))
	  (setq *event-types* (cons type-prefix *event-types*))

	  (output-header-file "_edsio")

	  (save-excursion
	    (goto-char *header-typedef-marker*)

	    (insert (format "/* %s%sEventCode.\n */\n\n" capprefix type-prefix))

	    (insert (format "typedef struct _%s%sEventCode %s%sEventCode;\n" capprefix type-prefix capprefix type-prefix))
	    (insert (format "struct _%s%sEventCode { gint code; };\n\n" capprefix type-prefix))

	    (insert (format "typedef struct _%s%sEvent %s%sEvent;\n" capprefix type-prefix capprefix type-prefix))
	    (insert (format "struct _%s%sEvent { gint code; const char* srcfile; guint srcline;%s%s };\n\n" capprefix type-prefix (event-struct-entries event) (event-struct-sys-entries event)))
	    )

	  (insert (format "void %s_generate_%s_event_internal (%s%sEventCode code, const char* srcfile, gint srcline%s);\n"
			  *output-prefix*
			  (downcase-string type-prefix)
			  capprefix
			  type-prefix
			  (event-uargs-plist uargs t)
			  ))
	  (insert (format "#define %s_generate_%s_event(ecode%s) %s_generate_%s_event_internal((ecode),__FILE__,__LINE__%s)\n\n"
			  *output-prefix*
			  (downcase-string type-prefix)
			  (event-uargs-alist uargs t)
			  *output-prefix*
			  (downcase-string type-prefix)
			  (event-uargs-mlist uargs t)))

	  (output-source-file "_edsio")

	  (insert (format "void\n%s_generate_%s_event_internal (%s%sEventCode _code, const char* _srcfile, gint _srcline%s)\n"
			  *output-prefix*
			  (downcase-string type-prefix)
			  capprefix
			  type-prefix
			  (event-uargs-plist uargs t)
			  ))
	  (insert (format "{\n"))
	  (insert (format "  %s%sEvent *_e = g_new0 (%s%sEvent, 1);\n" capprefix type-prefix capprefix type-prefix))
	  (insert (format "  _e->code = _code.code;\n  _e->srcline = _srcline;\n  _e->srcfile = _srcfile;\n"))
	  (insert (event-uargs-copy "_e" event))
	  (insert (event-sargs-copy "_e" event))
	  (insert (format "  eventdelivery_event_deliver ((GenericEvent*) _e);\n"))
	  (insert (format "}\n\n"))

	  ;; Field to string def

	  (unless (equal type-prefix "Void")
	    (save-excursion
	      (goto-char *source-top-marker*)
	      (insert (format "static const char* %s_%s_event_field_to_string (GenericEvent* ev, gint field);\n" capprefix type-prefix))
	      )

	    (insert (format "const char*\n%s_%s_event_field_to_string (GenericEvent* ev, gint field)\n"
			    capprefix type-prefix))
	    (insert (format "{\n"))

	    (unless (equal type-prefix (intern "Ssl"))
	      (insert (format "  %s%sEvent* it = (%s%sEvent*) ev;\n" capprefix type-prefix capprefix type-prefix)))
	    (insert (format "  switch (field)\n"))
	    (insert (format "    {\n"))

	    (let ((uargs (event-uargs-get event))
		  (i 0))
	      (while uargs
		(let ((uarg (car uargs)))
		  (insert (format "    case %d: return eventdelivery_%s_to_string (it->%s);\n" i (cadr uarg) (car uarg)))
		  )
		(setq i (+ i 1))
		(setq uargs (cdr uargs))
		)
	      )

	    (if (< 1 (length (event-sargs-get event)))
		(error "unhandled case, too many sargs"))

	    (when (event-sargs-get event)
	      (let ((sarg (car (event-sargs-get event))))
		(insert (format "    case %d: " (length (event-uargs-get event))))

		(if (not (member sarg '(ssl errno)))
		    (error "what type of sarg is %s" sarg))

		(if (eq sarg 'errno)
		    (insert (format "return g_strdup (g_strerror (it->ev_errno));\n")))

		(if (eq sarg 'ssl)
		    (insert (format "return eventdelivery_ssl_errors_to_string ();\n")))
		)
	      )

	      (insert (format "    default: abort ();\n"))
	      (insert (format "    }\n"))

	    (insert (format "}\n\n"))
	    )
	  )

	(output-header-file "_edsio")

	(insert (format "extern const %s%sEventCode EC_%s%s;\n"
			capprefix
			type-prefix
			capprefix
			(event-name-get event)))

	(insert (format "#define EC_%s%sValue ((%d<<EDSIO_LIBRARY_OFFSET_BITS)+%d)\n\n"
			capprefix
			(event-name-get event)
			*event-id*
			*library-id*))

	(output-source-file "_edsio")

	(insert (format "const %s%sEventCode EC_%s%s = { EC_%s%sValue };\n\n"
			capprefix
			type-prefix
			capprefix
			(event-name-get event)
			capprefix
			(event-name-get event)))


	(save-excursion
	  (goto-char *source-init-marker*)

	  (insert (format "  eventdelivery_initialize_event_def (EC_%s%sValue, EL_%s, %s, \"%s\", \"%s\", %s);\n"
			  capprefix
			  (event-name-get event)
			  (event-level-get event)
			  (event-flags-string event)
			  (event-name-get event)
			  (fixup-oneline event (event-desc-get event))
			  (if (equal type-prefix "Void")
			      "NULL"
			    (format "& %s_%s_event_field_to_string" capprefix type-prefix))))
	  )

	(setq *event-id* (+ 1 *event-id*))

	)

      (setq events (cdr events))
      )
    )
  )

(defun event-flags-string (event)
  (if (member 'ssl (event-sargs-get event))
      "EF_OpenSSL"
    "EF_None")
  )

(defun event-struct-entries (event)
  (apply (function concat)
	 (mapcar (function (lambda (x) (format " %s %s;" (event-type-to-ctype (cadr x)) (car x))))
		 (event-uargs-get event)))
  )

(defun event-struct-sys-entries (event)
  (if (member 'errno (event-sargs-get event))
      " gint ev_errno;"
    "")
  )

(defun event-uargs-copy (name event)
  (apply (function concat)
	 (mapcar (function (lambda (x) (format "  %s->%s = %s;\n" name (car x) (car x))))
		 (event-uargs-get event)))
  )

(defun event-sargs-copy (name event)
  (if (member 'errno (event-sargs-get event))
      (format "  %s->ev_errno = errno;\n" name)
    "")
  )

(defun event-type-to-ctype (etype)
  (let ((it (obj-name-eq etype *all-etype-defs*)))
    (if (not it)
	(message "no ctype for %s" etype))
    (etype-ctype-get it)
    )
  )

(defun event-uargs-plist(uargs need_first)
  (concat
   (if (and need_first uargs) ", " "")
   (format-comlist (function (lambda (x) (format "%s %s" (event-type-to-ctype (cadr x)) (car x)))) uargs))
  )

(defun event-uargs-alist(uargs need_first)
  (concat
   (if (and need_first uargs) ", " "")
   (format-comlist (function (lambda (x) (format "%s" (car x)))) uargs))
  )

(defun event-uargs-mlist(uargs need_first)
  (concat
   (if (and need_first uargs) ", " "")
   (format-comlist (function (lambda (x) (format "(%s)" (car x)))) uargs))
  )

(defun fixup-oneline (event oneline)
  (let ((work (get-buffer-create "*string-tmp2*")))
    (save-excursion
      (set-buffer work)
      (erase-buffer)
      (insert oneline)
      (beginning-of-buffer)

      (while (re-search-forward "${\\(\\w+\\)}" nil t)

 	(let* ((it    (intern (downcase-string (match-string 1))))
	       (uargs (event-uargs-get event))
	       (i     0)
	       (repl  nil))

	  (while uargs

	    (if (eq (car (car uargs)) it)
		(setq repl (format "${%d}" i)))

	    (setq uargs (cdr uargs))
	    (setq i (+ i 1))
	    )

	  (when (eq it 'strerror)
	    (if repl
		(error "No wildcards named STRERROR"))
	    (setq repl (format "${%d}" i))
	    )

	  (when (eq it 'ssl)
	    (if repl
		(error "No wildcards named SSL"))
	    (setq repl (format "${%d}" i))
	    )

	  (if (not repl)
	      (error "Illegal wildcard %s in %s" it oneline))

	  (replace-match repl nil nil)
 	  )
 	)

      (buffer-string)
      )
    )
  )

;; Properties

(defun generate-properties ()
  (let ((cap-prefix (capitalize1 *output-prefix*))
	(unique-types nil))
    (output-header-file "_edsio")

    (insert (format "/* Property definitions */\n\n"))

    ;; Types

    (output-source-file "_edsio")

    (mapcar
     (function
      (lambda (pht)
	(let ((type (prophosttype-type-get pht)))
	  (unless (member type unique-types)
	    (setq unique-types (cons type unique-types))

	    (save-excursion
	      (goto-char *source-init-marker*)

	      ;(message "%s -> %s %s" type (type-free-func type) (member type (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*)))

	      (insert (format "  edsio_initialize_property_type (\"%s\", %s, %s, %s, %s, %s);\n"
			      type
			      (type-free-func type)
			      (type-gs-func type "getter")
			      (type-gs-func type "setter")
			      (type-serialize-func type)
			      (type-unserialize-func type)))
	      )
	    )
	  )
	)
      )
     *prophosttype-defs*
     )

    ;; Host reg

    (mapcar
     (function
      (lambda (prophost)
	(save-excursion
	  (goto-char *source-init-marker*)
	  (insert (format "  edsio_initialize_host_type (\"%s\", %s, %s, %s, %s, %s);\n"
			  (prophost-name-get prophost)
			  (format "(PropertyTableFunc) & edsio_%s_property_table"
				  (downcase-string (prophost-name-get prophost)))
			  (prophost-persist prophost "source")
			  (prophost-persist prophost "sink")
			  (prophost-persist prophost "isset")
			  (prophost-persist prophost "unset")
			  ))
	  )
	)
      )
     *prophost-defs*)

    ;; Compute each distinct (host type) x (prop type)

    (mapcar
     (function
      (lambda (prophost)

	(mapcar
	 (function
	  (lambda (prophosttype)

	    (when (equal (prophosttype-host-get prophosttype) (prophost-name-get prophost))

	      (when (not (member (prophosttype-type-get prophosttype) (prophost-proptypes-get prophost)))
		(prophost-proptypes-set prophost (cons (prophosttype-type-get prophosttype) (prophost-proptypes-get prophost)))
		)
	      )))
	 *prophosttype-defs*
	 )

	;; Output the get/set functions for each property type

	(mapcar
	 (function
	  (lambda (type)

	    (let ((it (property-code-typename type prophost)))

	      ;; Header

	      (output-header-file "_edsio")

	      (insert (format "/* Property get/set for %s/%s\n */\n\n" (prophost-name-get prophost) type))

	      (insert (format "typedef struct _%s %s;\n" it it))

	      (insert (format "struct _%s { guint32 code; };\n\n" it))

	      (insert (format "gboolean edsio_new_%s_%s_property (const char* name, guint32 flags, %s* prop);\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      it
			      ))

	      (insert (format "gboolean %s_get_%s (%s obj, %s prop%s);\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      (prophost-ctype-get prophost)
			      it
			      (prop-type-to-get-fps type)))

	      (insert (format "gboolean %s_set_%s (%s obj, %s prop%s);\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      (prophost-ctype-get prophost)
			      it
			      (prop-type-to-set-fps type)))

	      (insert (format "gboolean %s_unset_%s (%s obj, %s prop);\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      (prophost-ctype-get prophost)
			      it))

	      (insert (format "gboolean %s_isset_%s (%s obj, %s prop);\n\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      (prophost-ctype-get prophost)
			      it))

	      ;; Source

	      (output-source-file "_edsio")

	      (insert (format "gboolean edsio_new_%s_%s_property (const char* name, guint32 flags, %s* prop)\n{\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      it
			      ))
	      (insert (format "  return edsio_new_property (name, \"%s\", \"%s\", flags, (EdsioGenericProperty*) prop);\n" (prophost-name-get prophost) type))
	      (insert (format "}\n\n"))

	      (insert (format "gboolean\n%s_get_%s (%s obj, %s prop%s)\n{\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      (prophost-ctype-get prophost)
			      it
			      (prop-type-to-get-fps type)))
	      (insert (format "  EdsioProperty* ep;\n"))
	      (insert (format "  g_return_val_if_fail (obj, FALSE);\n"))
	      (insert (format "  return (* edsio_property_getter (\"%s\", \"%s\", prop.code, & ep)) (obj, ep%s);\n"
			      (prophost-name-get prophost)
			      type
			      (prop-type-to-args type)
			      ))

	      (insert (format "}\n\n"))

	      (insert (format "gboolean\n%s_set_%s (%s obj, %s prop%s)\n{\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      (prophost-ctype-get prophost)
			      it
			      (prop-type-to-set-fps type)))
	      (insert (format "  EdsioProperty* ep;\n"))
	      (insert (format "  g_return_val_if_fail (obj, FALSE);\n"))
	      (insert (format "  return (* edsio_property_setter (\"%s\", \"%s\", prop.code, & ep)) (obj, ep%s);\n"
			      (prophost-name-get prophost)
			      type
			      (prop-type-to-args type)
			      ))

	      (insert (format "}\n\n"))

	      (insert (format "gboolean\n%s_unset_%s (%s obj, %s prop)\n{\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      (prophost-ctype-get prophost)
			      it))
	      (insert (format "  g_return_val_if_fail (obj, FALSE);\n"))
	      (insert (format "  return edsio_property_unset (\"%s\", \"%s\", prop.code, obj);\n"
			      (prophost-name-get prophost)
			      type
			      ""
			      ))

	      (insert (format "}\n\n"))

	      (insert (format "gboolean\n%s_isset_%s (%s obj, %s prop)\n{\n"
			      (downcase-string (prophost-name-get prophost))
			      (type-canon-name type)
			      (prophost-ctype-get prophost)
			      it))
	      (insert (format "  g_return_val_if_fail (obj, FALSE);\n"))
	      (insert (format "  return edsio_property_isset (\"%s\", \"%s\", prop.code, obj);\n"
			      (prophost-name-get prophost)
			      type
			      ))

	      (insert (format "}\n\n"))

	      )
	    )
	  )
	 (prophost-proptypes-get prophost)
	 )
	)
      )
     *all-prophost-defs*
     )
    )
  )

(defun property-code-typename(type prophost)
  (format "%s%s%sProperty"
	  (capitalize1 *output-prefix*)
	  (prophost-name-get prophost)
	  (capitalize1 type))
  )

(defun prop-typename-ctypes (type)
  (cond ((equal type 'string)
	 (list (list 'arg "const gchar*")))
	((equal type 'uint)
	 (list (list 'arg "guint32")))
	((equal type 'uint32)
	 (list (list 'arg "guint32")))
	((equal type 'uint16)
	 (list (list 'arg "guint16")))
	((equal type 'uint8)
	 (list (list 'arg "guint8")))
	((equal type 'boolean)
	 (list (list 'arg "gboolean")))
	((equal type 'bytes)
	 (list (list 'arg "const guint8*") (list 'arg_len "guint32")))
	((equal type 'object)
	 (list (list 'arg "void*") (list 'arg_type "guint32")))
	((member type (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
	 (list (list 'arg (format "Serial%s*" type))))
	((equal (car type) 'bytes)
	 (list (list 'arg "const guint8*")))
	((equal (car type) 'array)
	 (list (list 'arg (format "%s*" (cadr (car (prop-typename-ctypes (cadr type))))))
	       (list 'arg_len "guint32")))
	((equal (car type) 'ptr)
	 (list (list 'arg (format "%s*" (cadr (car (prop-typename-ctypes (cadr type))))))))
	(t (error "unrecognized field type: %s" type)))
  )

(defun prop-type-to-get-fps (type)
  (concat ", "
	  (format-comlist
	   (function
	    (lambda (pair)
	      (format "%s* %s" (cadr pair) (car pair))
	      )
	    )
	   (prop-typename-ctypes type))
	  )
  )

(defun prop-type-to-set-fps (type)
  (concat ", "
	  (format-comlist
	   (function
	    (lambda (pair)
	      (format "%s %s" (cadr pair) (car pair))
	      )
	    )
	   (prop-typename-ctypes type))
	  )
  )

(defun prop-type-to-args (type)
  (concat ", "
	  (format-comlist
	   (function
	    (lambda (pair)
	      (format "%s" (car pair))
	      )
	    )
	   (prop-typename-ctypes type))
	  )
  )

(defun type-canon-name (type)
  ; @@@ does not work for (array ...), etc
  (downcase-string type))

(defun type-serialize-func (type)
  (format "serialize_%s_obj" (downcase-string type))
  )

(defun type-unserialize-func (type)
  (format "unserialize_%s" (downcase-string type))
  )

(defun type-gs-func (type name)
  (if (member type (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
      (format "& edsio_property_vptr_%s" name)
    (format "& edsio_property_%s_%s" type name)))

(defun type-free-func (type)
  (if (member type (mapcar (lambda (x) (sertype-name-get x)) *all-sertype-defs*))
      (format "& edsio_property_vptr_free")
    (format "& edsio_property_%s_free" type)))

(defun prophost-persist (prophost func)
  (if (prophost-persist-get prophost)
      (format "(Persist%sFunc) & %s_persist_%s_%s"
	      (capitalize1 func)
	      *output-prefix*
	      (downcase-string (prophost-name-get prophost))
	      func)
    "NULL"))
