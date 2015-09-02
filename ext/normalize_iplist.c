
#include <ruby.h>

static VALUE module;

static int compare_serialized(const void *a_, const void *b_)
{
    const uint8_t *a = a_, *b = b_;
    for (size_t i = 0; i < 5; ++i)
	if (a[i]-b[i]) return a[i]-b[i];
    return 0;
}

struct ip_and_mask {
    uint32_t ip;
    uint8_t mask;
};

struct ip_and_mask read_textual_ip_address_and_optional_mask(char *p)
{
    uint8_t a,b,c,d;
    struct ip_and_mask ip_and_mask = {0};
    int rv = sscanf(p, "%hhu.%hhu.%hhu.%hhu/%hhu", &a, &b, &c, &d, &ip_and_mask.mask);
    switch (rv) {
    case 4:
	ip_and_mask.mask = 32;
    case 5:
	ip_and_mask.ip = a<<24 | b<<16 | c<<8 | d;
	return ip_and_mask;
    default:
	ip_and_mask.mask = 0;
	return ip_and_mask;
    }
}

            /* serialized = row[:ips].map do |cidr| */
            /*   (ip, mask) = cidr.split('/') */
            /*   (ip.split('.').map(&:to_i) << (mask || 32).to_i).pack('CCCCC') */
            /* end.sort.join */
/* Serialize an array of IPs as strings into a single string of big-endian
 * integers plus mask. */
static VALUE serialize(VALUE self, VALUE in) {
    if (TYPE(in) != T_ARRAY)
	rb_raise(rb_eTypeError, "invalid type; serialize requires an array");

    size_t n = RARRAY_LEN(in);
    VALUE out = rb_str_new(NULL, n*5);
    VALUE out_str = rb_string_value(&out);
    char *out_p = RSTRING_PTR(out_str);

    for (size_t i = 0; i < n; ++i) {
	VALUE ent = rb_ary_entry(in, i);
	char *s = StringValueCStr(ent);
	struct ip_and_mask ip_and_mask = read_textual_ip_address_and_optional_mask(s);
	if (ip_and_mask.mask < 8 || ip_and_mask.mask > 32)
	    rb_raise(rb_eArgError, "invalid IP and mask");
	uint32_t ip = ip_and_mask.ip;
	uint8_t mask = ip_and_mask.mask;
	ip &= (uint32_t)0xffffffff << (32-mask);
	*out_p++ = ip>>24; *out_p++ = ip>>16; *out_p++ = ip>>8; *out_p++ = ip;
	*out_p++ = ip_and_mask.mask;
    }
    qsort(RSTRING_PTR(out_str), n, 5, compare_serialized);
    return out;
}

enum { MAX_IP_FMT_LEN = 4*4+4 };

static void format_ip_and_mask(uint8_t *in, char *out)
{
    if (in[4] == 32)
	snprintf(out, MAX_IP_FMT_LEN, "%hhu.%hhu.%hhu.%hhu", in[0], in[1], in[2], in[3]);
    else
	snprintf(out, MAX_IP_FMT_LEN, "%hhu.%hhu.%hhu.%hhu/%hhu", in[0], in[1], in[2], in[3], in[4]);
}

      /* self.ips = ips.map { |ip| self.class.string_to_cidr(ip) }.uniq.sort.map { |ip| self.class.cidr_to_string(ip) } */
/* Normalize IP addresses, uniq and sort, return array. */
static VALUE normalize_text(VALUE self, VALUE in) {
    VALUE sorted_bin = serialize(self, in);
    uint8_t *p = (uint8_t *)RSTRING_PTR(sorted_bin);
    VALUE out = rb_ary_new();
    size_t n = RSTRING_LEN(sorted_bin)/5;
    if (n == 0) return out;

    char buf[MAX_IP_FMT_LEN+1] = {0};
    format_ip_and_mask(p, buf);
    rb_ary_push(out, rb_str_new2(buf));
    p += 5;
    for (size_t i = 1; i < n; ++i, p += 5) {
	if (p[0] == p[-5] && p[1] == p[-4] && p[2] == p[-3] && p[3] == p[-2] && p[4] == p[-1])
	    continue;
	format_ip_and_mask(p, buf);
	rb_ary_push(out, rb_str_new2(buf));
    }
    return out;
}

void Init_normalize_iplist(void) {
    /* VALUE cNormalizeIplist = rb_const_get(rb_cObject, rb_intern("NormalizeIPList")); */
    module = rb_define_module("NormalizeIPList");
    rb_define_module_function(module, "normalize_text", normalize_text, 1);
    rb_define_module_function(module, "serialize", serialize, 1);
}
